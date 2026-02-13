#pragma once

// Support for SVGPatternElement
// http://www.w3.org/TR/SVG11/feature#Pattern
// pattern
//

#include <memory>
#include <vector>
#include <cmath>
#include <cstdint>


#include "svgstructuretypes.h"
#include "viewport.h"
#include "svgb2ddriver.h"


namespace waavs
{
    struct SVGPatternElement; // forward declaration


	// The state of a pattern as found in the document
	// before binding occurs
	struct DocPatternState
	{
		// <length> / <percentage>
		SVGLengthValue x{};
		SVGLengthValue y{};
		SVGLengthValue width{};
		SVGLengthValue height{};

		// Enums
		SpaceUnitsKind patternUnits{ SpaceUnitsKind::SVG_SPACE_OBJECT };        // objectBoundingBox default
		SpaceUnitsKind patternContentUnits{ SpaceUnitsKind::SVG_SPACE_USER };   // userSpaceOnUse default
		BLExtendMode extendMode{ BL_EXTEND_MODE_REPEAT };

		// Optional transforms
		bool hasPatternTransform{ false };
		BLMatrix2D patternTransform; // { BLMatrix2D::makeIdentity() };

		// viewBox/par
		bool hasViewBox{ false };
		BLRect viewBox;
		PreserveAspectRatio par;

		// href template (raw string span)
		waavs::ByteSpan href;
	};

	static INLINE bool resolvePatternDocWithHref(const DocPatternState& authored, IAmGroot* groot, DocPatternState& outFinal) noexcept;



	// The state of a patter after binding occurs.
	struct ResolvedPatternState
	{
		// Tile rectangle in pattern user space.  This is the area that will be repeated to fill the pattern.
		BLRect tile{};

		// Transform to apply on a scratch context when rendering tile pixels.
		// This maps "pattern content user space" to "tile image pixel space".
		BLMatrix2D contentToTilePx{};

		// Transform to set on BLPattern
		// (placement / phase / author patternTransform).
		BLMatrix2D patternMatrix{};
		bool resolved{ false };
	};

	// some small helpers
	static INLINE int clampTilePx(int v) noexcept { return v < 1 ? 1 : v; }

	static INLINE uint64_t hashU64(uint64_t h, uint64_t v) noexcept
	{
		// simple mix, not cryptographic
		h ^= v + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
		return h;
	}

	static INLINE uint64_t hashDbl(uint64_t h, double d) noexcept
	{
		union { double d; uint64_t u; } x{ d };
		return hashU64(h, x.u);
	}

	static INLINE uint64_t hashRect(uint64_t h, const BLRect& r) noexcept
	{
		h = hashDbl(h, r.x);
		h = hashDbl(h, r.y);
		h = hashDbl(h, r.w);
		h = hashDbl(h, r.h);
		return h;
	}

	static INLINE uint64_t hashMatrix(uint64_t h, const BLMatrix2D& m) noexcept
	{
		h = hashDbl(h, m.m00); h = hashDbl(h, m.m01);
		h = hashDbl(h, m.m10); h = hashDbl(h, m.m11);
		h = hashDbl(h, m.m20); h = hashDbl(h, m.m21);
		return h;
	}

	static INLINE uint64_t hashDocPattern(const DocPatternState& d) noexcept
	{
		uint64_t h = 0xcbf29ce484222325ULL;

		h = hashDbl(h, d.x.value());      h = hashU64(h, d.x.unitType());      h = hashU64(h, d.x.isSet());
		h = hashDbl(h, d.y.value());      h = hashU64(h, d.y.unitType());      h = hashU64(h, d.y.isSet());
		h = hashDbl(h, d.width.value());  h = hashU64(h, d.width.unitType());  h = hashU64(h, d.width.isSet());
		h = hashDbl(h, d.height.value()); h = hashU64(h, d.height.unitType()); h = hashU64(h, d.height.isSet());

		h = hashU64(h, uint64_t(d.patternUnits));
		h = hashU64(h, uint64_t(d.patternContentUnits));
		h = hashU64(h, uint64_t(d.extendMode));

		h = hashU64(h, uint64_t(d.hasPatternTransform));
		if (d.hasPatternTransform) h = hashMatrix(h, d.patternTransform);

		h = hashU64(h, uint64_t(d.hasViewBox));
		if (d.hasViewBox) h = hashRect(h, d.viewBox);

		h = hashU64(h, uint64_t(d.par.align()));
		h = hashU64(h, uint64_t(d.par.meetOrSlice()));

		// href is not hashed (doc merge already applied); leaving it out is fine.
		return h;
	}

	static INLINE void mergeIfUnset(DocPatternState& dst, const DocPatternState& src) noexcept
	{
		if (!dst.x.isSet() && src.x.isSet())     dst.x = src.x;
		if (!dst.y.isSet() && src.y.isSet())     dst.y = src.y;
		if (!dst.width.isSet() && src.width.isSet()) dst.width = src.width;
		if (!dst.height.isSet() && src.height.isSet())dst.height = src.height;

		// enums: only override if dst still default AND src differs (template provides explicit).
		// (Simple rule: always take src if dst is default construction and src came from authored.)
		// Here: take src unconditionally if dst has not explicitly set the attribute.
		// Since we don't track "isSet" for enums, we just merge only when the attribute is absent
		// at parse time. We'll implement that by setting enums during parse only when attribute exists.
		// (So this merge assumes parse does NOT overwrite defaults unless attribute present.)
		// Thus: just copy.
		dst.patternUnits = src.patternUnits;
		dst.patternContentUnits = src.patternContentUnits;
		dst.extendMode = src.extendMode;

		if (!dst.hasPatternTransform && src.hasPatternTransform) {
			dst.hasPatternTransform = true;
			dst.patternTransform = src.patternTransform;
		}

		if (!dst.hasViewBox && src.hasViewBox) {
			dst.hasViewBox = true;
			dst.viewBox = src.viewBox;
		}

		// preserveAspectRatio: always merge (default is fine)
		dst.par = src.par;
	}


}

namespace waavs
{
	// ---------------------------- parsing doc state ----------------------------

	static  void loadDocPatternState(DocPatternState& out, const XmlAttributeCollection& attrs) noexcept
	{
		out = DocPatternState{};

		// href / xlink:href
		ByteSpan href{};
		attrs.getValue(svgattr::href(), href);
		if (!href) attrs.getValue(svgattr::xlink_href(), href);
		out.href = chunk_trim(href, chrWspChars);

		// x,y,w,h
		ByteSpan xA{}, yA{}, wA{}, hA{};
		attrs.getValue(svgattr::x(), xA);
		attrs.getValue(svgattr::y(), yA);
		attrs.getValue(svgattr::width(), wA);
		attrs.getValue(svgattr::height(), hA);

		out.x = parseLengthAttr(xA);
		out.y = parseLengthAttr(yA);
		out.width = parseLengthAttr(wA);
		out.height = parseLengthAttr(hA);

		// patternUnits / patternContentUnits (only if present; otherwise keep defaults)
		ByteSpan puA{}, pcuA{};
		if (attrs.getValue(svgattr::patternUnits(), puA)) {
			uint32_t v{};
			if (getEnumValue(SVGSpaceUnits, puA, v)) out.patternUnits = (SpaceUnitsKind)v;
		}
		if (attrs.getValue(svgattr::patternContentUnits(), pcuA)) {
			uint32_t v{};
			if (getEnumValue(SVGSpaceUnits, pcuA, v)) out.patternContentUnits = (SpaceUnitsKind)v;
		}

		// extendMode (your custom)
		ByteSpan emA{};
		if (attrs.getValue(svgattr::extendMode(), emA)) {
			uint32_t v{};
			if (getEnumValue(SVGExtendMode, emA, v)) out.extendMode = (BLExtendMode)v;
		}

		// patternTransform
		ByteSpan ptA{};
		if (attrs.getValue(svgattr::patternTransform(), ptA)) {
			out.hasPatternTransform = parseTransform(ptA, out.patternTransform);
		}

		// preserveAspectRatio
		ByteSpan parA{};
		if (attrs.getValue(svgattr::preserveAspectRatio(), parA)) {
			out.par.loadFromChunk(parA);
		}

		// viewBox
		ByteSpan vbA{};
		BLRect vb{};
		if (attrs.getValue(svgattr::viewBox(), vbA) && parseViewBox(vbA, vb)) {
			out.hasViewBox = true;
			out.viewBox = vb;
		}
	}





	// ---------------------------- resolve per-use ----------------------------

	static bool resolvePatternState(
		const BLRect& paintVP,
		const BLRect& objBBox,
		const DocPatternState& doc,
		double dpi,
		const BLFont* fontOpt,
		double deviceScaleX,
		double deviceScaleY,
		ResolvedPatternState& out) noexcept
	{
		out = ResolvedPatternState{};

		if (paintVP.w <= 0 || paintVP.h <= 0)
			return false;

		const bool puObject = (doc.patternUnits == SpaceUnitsKind::SVG_SPACE_OBJECT);

		// Resolve tile x/y/w/h
		LengthResolveCtx cx{}, cy{}, cw{}, ch{};
		if (puObject) {
			cx = makeLengthCtxUser(objBBox.w, 0.0, dpi, fontOpt); cx.space = SpaceUnitsKind::SVG_SPACE_OBJECT;
			cy = makeLengthCtxUser(objBBox.h, 0.0, dpi, fontOpt); cy.space = SpaceUnitsKind::SVG_SPACE_OBJECT;
			cw = cx; ch = cy;
		}
		else {
			cx = makeLengthCtxUser(paintVP.w, 0.0, dpi, fontOpt);
			cy = makeLengthCtxUser(paintVP.h, 0.0, dpi, fontOpt);
			cw = cx; ch = cy;
		}

		const double x = resolveLengthOr(doc.x, cx, 0.0) + (puObject ? objBBox.x : paintVP.x);
		const double y = resolveLengthOr(doc.y, cy, 0.0) + (puObject ? objBBox.y : paintVP.y);
		const double w = resolveLengthOr(doc.width, cw, 0.0);
		const double h = resolveLengthOr(doc.height, ch, 0.0);

		if (w <= 0.0 || h <= 0.0)
			return false;

		out.tile = BLRect{ x, y, w, h };

		// Decide tile pixel size using device scale (quality)
		const int tilePxW = clampTilePx(int(std::ceil(w * (deviceScaleX > 0 ? deviceScaleX : 1.0))));
		const int tilePxH = clampTilePx(int(std::ceil(h * (deviceScaleY > 0 ? deviceScaleY : 1.0))));

		// tileUser -> tilePx
		BLMatrix2D tileUserToPx = BLMatrix2D::makeIdentity();
		tileUserToPx.scale(double(tilePxW) / w, double(tilePxH) / h);

		// content -> tileUser
		BLMatrix2D contentToTileUser = BLMatrix2D::makeIdentity();

		if (doc.hasViewBox)
		{
			// viewBox defines content coords
			BLMatrix2D vb2tile{};
			if (!computeViewBoxToViewport(BLRect{ 0,0,w,h }, doc.viewBox, doc.par, vb2tile))
				return false;
			contentToTileUser = vb2tile;
		}
		else
		{
			// no viewBox: content coords depend on patternContentUnits
			if (doc.patternContentUnits == SpaceUnitsKind::SVG_SPACE_USER)
			{
				// content in paint user space -> tile local
				contentToTileUser.translate(-out.tile.x, -out.tile.y);
			}
			else // objectBoundingBox
			{
				// content in bbox fraction space: (0..1)->bbox in paint user space -> tile local
				contentToTileUser.translate(objBBox.x, objBBox.y);
				contentToTileUser.scale(objBBox.w, objBBox.h);
				contentToTileUser.translate(-out.tile.x, -out.tile.y);
			}
		}

		// content -> tilePx
		out.contentToTilePx = tileUserToPx;
		out.contentToTilePx.postTransform(contentToTileUser);

		// pattern matrix (phase + patternTransform)
		out.patternMatrix = BLMatrix2D::makeIdentity();
		out.patternMatrix.translate(-out.tile.x, -out.tile.y);
		if (doc.hasPatternTransform)
			out.patternMatrix.transform(doc.patternTransform);

		out.resolved = true;
		return true;
	}

}


namespace waavs
{
	//============================================================
	// SVGPatternElement
	// https://www.svgbackgrounds.com/svg-pattern-guide/#:~:text=1%20Background-size%3A%20cover%3B%20This%20declaration%20is%20good%20when,5%20Background-repeat%3A%20repeat%3B%206%20Background-attachment%3A%20scroll%20%7C%20fixed%3B
	// https://www.svgbackgrounds.com/category/pattern/
	// https://www.visiwig.com/patterns/
	//============================================================

	struct SVGPatternElement : public SVGGraphicsElement
	{
		static void registerSingularNode()
		{
			registerSVGSingularNodeByName("pattern", [](IAmGroot* groot, const XmlElement& elem) {
				auto node = std::make_shared<SVGPatternElement>(groot);
				node->loadFromXmlElement(elem, groot);
				return node;
				});
		}

		static void registerFactory()
		{
			registerContainerNodeByName("pattern",
				[](IAmGroot* groot, XmlPull& iter) {
					auto node = std::make_shared<SVGPatternElement>(groot);
					node->loadFromXmlPull(iter, groot);
					return node;
				});
			registerSingularNode();
		}

		// ---- Stored doc + cache ----
		DocPatternState fDoc{};
		bool fHasDoc{ false };

		// cache key inputs
		uint64_t fDocHash{ 0 };
		BLRect fLastPaintVP{};
		BLRect fLastObjBBox{};
		double fLastDpi{ 96.0 };
		double fLastScaleX{ 1.0 };
		double fLastScaleY{ 1.0 };

		// resolved + cache
		ResolvedPatternState fResolved{};
		BLImage  fTileImage{};
		BLPattern fPattern{};

		SVGPatternElement(IAmGroot*) : SVGGraphicsElement()
		{
			setIsStructural(true);
            setIsVisible(false); // never directly visible; only via paint server

			fPattern.setExtendMode(BL_EXTEND_MODE_REPEAT);
		}

		// Expose doc to templates resolver
		void getDocPatternState(DocPatternState& out) const noexcept
		{
			out = fDoc;
		}

		// Fixup after style application: parse doc state only (no resolving here).
		void fixupSelfStyleAttributes(IAmGroot*) override
		{
			loadDocPatternState(fDoc, fAttributes);
			fHasDoc = true;
		}

		// Pattern is used as a paint server: return variant
		const BLVar getVariant(IRenderSVG* ctx, IAmGroot* groot) noexcept override
		{
			// Resolve + update cache if needed
			(void)ensurePatternReady(ctx, groot);

			BLVar v{};
			v = fPattern;
			return v;
		}

		// Not a geometry bbox; return something stable
		BLRect objectBoundingBox() const override
		{
			return fResolved.resolved ? fResolved.tile : BLRect{};
		}

	private:
		// We need some way to estimate device scale for quality.
		static INLINE void getDeviceScaleEstimate(IRenderSVG* ctx, double& sx, double& sy) noexcept
		{
			sx = 1.0;
			sy = 1.0;
			if (!ctx) 
				return;

			const BLMatrix2D m = ctx->getTransform();

			// Column magnitudes (scale of unit vectors)
			const double ax = m.m00;
			const double bx = m.m01;
			const double ay = m.m10;
			const double by = m.m11;

			const double xLen = std::hypot(ax, bx);
			const double yLen = std::hypot(ay, by);

			// Guard against degenerate transforms.
			if (xLen > 1e-12) sx = xLen;
			if (yLen > 1e-12) sy = yLen;

			// Optional: if you want “uniform-ish” scale for rasterization:
			// const double s = std::max(sx, sy); sx = sy = s;
		}

		void bindPatternAfterRender(
			const DocPatternState& docFinal,
			const ResolvedPatternState& rs,
			int tilePxW,
			int tilePxH) noexcept
		{
			// USER -> PIXEL scale (this was inverted before)
			const double uxToPx = double(tilePxW) / rs.tile.w;
			const double uyToPx = double(tilePxH) / rs.tile.h;

			BLMatrix2D m = BLMatrix2D::makeIdentity();

			// 1) map user space to image pixel space
			m.scale(uxToPx, uyToPx);

			// 2) phase (tile origin) in user units
			m.translate(-rs.tile.x, -rs.tile.y);

			// 3) authored patternTransform (user space)
			if (docFinal.hasPatternTransform)
				m.transform(docFinal.patternTransform);

			fPattern.setImage(fTileImage);
			fPattern.setExtendMode(docFinal.extendMode);
			fPattern.setTransform(m);
		}






		bool ensurePatternReady(IRenderSVG* ctx, IAmGroot* groot) noexcept
		{
			if (!ctx || !fHasDoc)
				return false;

			DocPatternState docFinal{};
			resolvePatternDocWithHref(fDoc, groot, docFinal);

			const BLRect paintVP = ctx->viewport();
			const BLRect objBBox = ctx->getObjectFrame();
			//const double dpi = groot ? groot->dpi() : 96.0;
			const double dpi = 96.0;

			double devSx = 1.0, devSy = 1.0;
			getDeviceScaleEstimate(ctx, devSx, devSy);

			const uint64_t h = hashDocPattern(docFinal);

			const bool needsResolve =
				(!fResolved.resolved) ||
				(h != fDocHash) ||
				(dpi != fLastDpi) ||
				(devSx != fLastScaleX) || (devSy != fLastScaleY) ||
				(paintVP.x != fLastPaintVP.x || paintVP.y != fLastPaintVP.y ||
					paintVP.w != fLastPaintVP.w || paintVP.h != fLastPaintVP.h) ||
				(objBBox.x != fLastObjBBox.x || objBBox.y != fLastObjBBox.y ||
					objBBox.w != fLastObjBBox.w || objBBox.h != fLastObjBBox.h);

			if (!needsResolve)
				return true;

			ResolvedPatternState rs{};
			const BLFont* font = &ctx->getFont();
			if (!resolvePatternState(paintVP, objBBox, docFinal, dpi, font, devSx, devSy, rs))
				return false;

			const int tilePxW = clampTilePx(int(std::ceil(rs.tile.w * (devSx > 0 ? devSx : 1.0))));
			const int tilePxH = clampTilePx(int(std::ceil(rs.tile.h * (devSy > 0 ? devSy : 1.0))));

			if (!renderTile(groot, docFinal, rs, tilePxW, tilePxH))
				return false;

			bindPatternAfterRender(docFinal, rs, tilePxW, tilePxH);

			fResolved = rs;
			fDocHash = h;
			fLastPaintVP = paintVP;
			fLastObjBBox = objBBox;
			fLastDpi = dpi;
			fLastScaleX = devSx;
			fLastScaleY = devSy;

			return true;
		}



		bool renderTile(
			IAmGroot* groot,
			const DocPatternState& docFinal,
			const ResolvedPatternState& rs,
			int tilePxW,
			int tilePxH) noexcept
		{
			// Allocate & clear
			fTileImage.create(tilePxW, tilePxH, BL_FORMAT_PRGB32);

			SVGB2DDriver ictx;
			ictx.attach(fTileImage);
			ictx.renew();
			ictx.clear();

			// Nearest viewport for pattern children is the TILE USER RECT.
			ictx.setViewport(BLRect{ 0.0, 0.0, rs.tile.w, rs.tile.h });

			// Object frame for children:
			// - If patternContentUnits==objectBoundingBox and no viewBox: children content space is 0..1
			// - Otherwise: treat like tile user space
			if (docFinal.patternContentUnits == SpaceUnitsKind::SVG_SPACE_OBJECT && !docFinal.hasViewBox)
				ictx.setObjectFrame(BLRect{ 0.0, 0.0, 1.0, 1.0 });
			else
				ictx.setObjectFrame(BLRect{ 0.0, 0.0, rs.tile.w, rs.tile.h });

			// Map pattern-content user coords -> tile pixels.
			// IMPORTANT: setTransform, not applyTransform, so we start clean.
			ictx.setTransform(rs.contentToTilePx);

			// Draw children into the tile image.
			drawChildren(&ictx, groot);

			ictx.flush();
			ictx.detach();
			return true;
		}





		// Resolve template chain into final doc state (no attribute mutation).
		static INLINE bool resolvePatternDocWithHref(
			const DocPatternState& authored,
			IAmGroot* groot,
			DocPatternState& outFinal) noexcept
		{
			outFinal = authored;

			if (!groot || !authored.href)
				return true;

			// follow up to N levels to avoid cycles
			DocPatternState cur = authored;
			for (int depth = 0; depth < 8; ++depth)
			{
				if (!cur.href) break;

				auto node = groot->findNodeByHref(cur.href);
				if (!node) break;

				auto patt = std::dynamic_pointer_cast<SVGPatternElement>(node);
				if (!patt) break;

				//patt->ensureDocReady(groot);

				// Ensure the referenced pattern has its doc parsed (style already merged).
				// We'll just ask it for its doc state via a helper (see below).
				DocPatternState templ{};
				patt->getDocPatternState(templ);

				// Merge: only fill unset fields. For enums, templ overwrites only if it was explicitly authored;
				// in this reference we keep it simple and merge unconditionally (good enough once you preserve
				// enum "isSet" if you want strictness).
				mergeIfUnset(outFinal, templ);

				// Continue chain from template's href (SVG allows href chains)
				cur = templ;
			}

			return true;
		}


	};

}