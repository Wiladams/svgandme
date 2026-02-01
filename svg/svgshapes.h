#pragma once

#include <string>
#include <array>
#include <list>
#include <functional>
#include <unordered_map>

#include "svgattributes.h"
#include "b2dpathbuilder.h"
#include "svgtext.h"
#include "viewport.h"
#include "svgmarker.h"


namespace waavs {
    // A helper to paint a path with fill/stroke/markers
	// deals with the paint order
	struct PathPainter
	{
		IRenderSVG* ctx;
		IAmGroot* groot;
		const BLPath& path;

		// Optional: hook markers back to the owning element (or a helper).
		// If nullptr, markers are a no-op.
		void (*drawMarkersFn)(void* user, IRenderSVG* ctx, IAmGroot* groot) { nullptr };
		void* markersUser{ nullptr };

		// Same semantics as your SVGPathBasedGeometry::drawSelf():
		// begin/end are outside paint-order ops so markers can be drawn between.
		void begin() noexcept { ctx->beginDrawShape(path); }
		void end()   noexcept { ctx->endDrawShape(); }

		void onFill()    noexcept { ctx->fillShape(path); }
		void onStroke()  noexcept { ctx->strokeShape(path); }
		void onMarkers() noexcept
		{
			if (drawMarkersFn)
				drawMarkersFn(markersUser, ctx, groot);
		}
	};

} // namespace waavs



namespace waavs {




	// SVGPathBasedGeometry
	//
	// All SVG shapes are represented using the BLPath
	// ultimately.  This serves as a base object for all the shapes
	// It handles the various attributes, and drawing of markers
	//
	struct SVGPathBasedGeometry : public SVGGraphicsElement
	{
		BLPath fPath{};
		bool fHasMarkers{ false };

		SVGPathBasedGeometry(IAmGroot* iMap)
			:SVGGraphicsElement()
		{
		}

		// Return bounding rectangle for shape
		// This does not include the stroke width
		BLRect viewPort() const override
		{
			// get bounding box, then apply transform
			BLRect bbox = getBBox();
			return bbox;
		}

		BLRect getBBox() const override
		{
			BLBox bbox{};
			fPath.getBoundingBox(&bbox);

			// if we have markers turned on, add in the bounding
			// box of the markers

			return BLRect(bbox.x0, bbox.y0, (bbox.x1 - bbox.x0), (bbox.y1 - bbox.y0));
		}

		bool contains(double x, double y) override
		{
			BLPoint localPoint(x, y);

			// BUGBUG - do we need to check for a transform, and transform the point?
			// check to see if we have a transform property
			// if we do, transform the points through that transform
			// then check to see if the point is inside the transformed path
			//if (fHasTransform)
			//{
				// get the inverse transform
			//	auto inverse = fTransform;
			//	inverse.invert();
			//	localPoint = inverse.mapPoint(localPoint);
			//}

			
			// BUGBUG - should use actual fill rule
			BLHitTest ahit = fPath.hitTest(localPoint, BLFillRule::BL_FILL_RULE_EVEN_ODD);
			
			return (ahit == BLHitTest::BL_HIT_TEST_IN);
		}
		
		bool checkForMarkers()
		{
			// figure out if we have any markers set
			auto mStart = getAttribute(svgattr::marker_start());
			auto mMid = getAttribute(svgattr::marker_mid());
			auto mEnd = getAttribute(svgattr::marker_end());
			auto m = getAttribute(svgattr::marker());
			
			if ((mStart&& mStart!=svgval::none()) || (mMid&& mMid!=svgval::none()) || (mEnd&&mEnd!=svgval::none()) || (m&& m!=svgval::none())) 
			{
				fHasMarkers = true;
			}
			
			return fHasMarkers;
		}

		void fixupSelfStyleAttributes(IRenderSVG* , IAmGroot* ) override
		{
			checkForMarkers();
		}
		
		//
		// drawMarker
		//
		// Draw a single marker along the path.
		// propname - specifies the kind of marker
		// pos - specifies the position of the marker

		bool drawMarker(IRenderSVG* ctx, IAmGroot* groot, InternedKey propKey, MarkerPosition pos, const BLPoint& p1, const BLPoint& p2, const BLPoint& p3)
		{
			std::shared_ptr<SVGVisualProperty> prop = getVisualProperty(propKey);
			
			// Look for the default marker if the specified one is not found
			if (nullptr == prop)
			{
				prop = getVisualProperty(svgattr::marker());
				if (nullptr == prop)
					return false;
			}
			
			// cast the prop to a SVGMarkerAttribute
			auto marker = std::dynamic_pointer_cast<SVGMarkerAttribute>(prop);

			if (nullptr == marker)
				return false;

			// get the marker node, as shared_ptr<IViewable>
			auto aNode = marker->markerNode(ctx, groot);
			if (nullptr == aNode)
				return false;

			// cast the node to a SVGMarkerNode
			std::shared_ptr<SVGMarkerElement> markerNode = std::dynamic_pointer_cast<SVGMarkerElement>(aNode);



			BLPoint transP{};
			BLRgba32 transC{};

			switch (pos)
			{
			case MarkerPosition::MARKER_POSITION_START:
				transP = p1;
				transC = BLRgba32(0x7fff0000);

				break;
			case MarkerPosition::MARKER_POSITION_MIDDLE:
				transP = p2;
				transC = BLRgba32(0x7f00ff00);
				break;
			case MarkerPosition::MARKER_POSITION_END:
				transP = p2;
				transC = BLRgba32(0x7f0000ff);
				break;
			}

			// BUGBUG - red (start), green (mid), blue(end)
			//ctx->fillCircle(transP.x, transP.y, 2, transC);



			// Use the three given points to calculate the angle of rotation 
			// from the markerNode
			double rads = markerNode->orientation().calcRadians(pos, p1, p2, p3);
			
			BLRect objectFrame = markerNode->getBBox();
			//BLRect viewport = ctx->viewport();
			
			ctx->push();
			//ctx->objectFrame(objectFrame);
			
			ctx->translate(transP);
			ctx->rotate(rads);

			// draw the marker
			markerNode->draw(ctx, groot);


			ctx->pop();

			return true;
		}


		void drawMarkers(IRenderSVG* ctx, IAmGroot* groot)
		{
			// early return if we don't have markers
			if (!fHasMarkers)
				return;


			
			static const uint8_t CMD_INVALID = 0xffu;

			ByteSpan cmdSpan(fPath.commandData(), fPath.commandDataEnd());

			const BLPoint* verts = fPath.vertexData();
			size_t numVerts = fPath.size();
			int vertOffset{ 0 };

			// Array of 3 points which represent current vectors
			BLPoint vecpts[3]{};

			uint8_t lastCmd = CMD_INVALID;
			BLPoint lastMoveTo{};
			BLPoint lastOnPoint{};

			//printf("Command Size: %d  Path Size: %d\n", cmdSpan.size(), fPath.size());
			ctx->push();
			
			while (cmdSpan)
			{
				int nVerts = 0;
				uint8_t cmd = cmdSpan[0];
				//printf("COORD: %d %f, %f\n", cmd, verts[vertOffset].x, verts[vertOffset].y);

				switch (cmd)
				{
					// For the MOVE command, it's a first point, so in order to calculate an angle
					// we need a following point, if it exists.  If it doesn't exist, then we 
					// just use the same point twice
				case BL_PATH_CMD_MOVE: {
					// how many points/commands to consume after this
					nVerts = 1;

					vecpts[0] = verts[vertOffset];
					vecpts[1] = verts[vertOffset];
					vecpts[2] = verts[vertOffset];

					lastMoveTo = verts[vertOffset];
					lastOnPoint = verts[vertOffset];

					// If there is a next command, decide the second point
					// based on what that command might be.
					if (cmdSpan.size() > 1)
					{
						vecpts[1] = verts[vertOffset + nVerts];

						uint8_t nextCmd = cmdSpan[1];
						switch (nextCmd) {
						case BL_PATH_CMD_ON:
						case BL_PATH_CMD_CUBIC:
						case BL_PATH_CMD_QUAD:
							//vecpts[1] = verts[vertOffset + 1];
							break;

						case BL_PATH_CMD_CLOSE:
						case BL_PATH_CMD_MOVE:
						default:
							vecpts[1] = vecpts[0];
						}
					}

					drawMarker(ctx, groot, svgattr::marker_start(), MarkerPosition::MARKER_POSITION_START, vecpts[0], vecpts[1], vecpts[2]);

					lastCmd = BL_PATH_CMD_MOVE;
				}
				break;


				// This is a complex case.  
				// Normally, we'd see a CMD_ON after a CMD_MOVE, but we could see a CMD_ON after a CMD_CUBIC
				case BL_PATH_CMD_ON: {
					nVerts = 1; // number of Vertices to consume

					vecpts[0] = lastOnPoint;
					vecpts[1] = verts[vertOffset];
					vecpts[2] = verts[vertOffset];


					// If there is another command, then we can decide the third point
					if (cmdSpan.size() > 1)
					{
						uint8_t nextCmd = cmdSpan[1];
						switch (nextCmd) {
						case BL_PATH_CMD_ON:
						case BL_PATH_CMD_CUBIC:
							vecpts[2] = verts[vertOffset + nVerts];
							drawMarker(ctx, groot, svgattr::marker_mid(), MarkerPosition::MARKER_POSITION_MIDDLE, vecpts[0], vecpts[1], vecpts[2]);
							lastOnPoint = vecpts[1];
							break;

						case BL_PATH_CMD_CLOSE:
							vecpts[2] = lastMoveTo;
							drawMarker(ctx, groot, svgattr::marker_mid(), MarkerPosition::MARKER_POSITION_MIDDLE, vecpts[0], vecpts[1], vecpts[2]);
							lastOnPoint = vecpts[1];
							break;

						case BL_PATH_CMD_MOVE:
						default:
							vecpts[2] = vecpts[1];
							drawMarker(ctx, groot, svgattr::marker_end(), MarkerPosition::MARKER_POSITION_END, verts[vertOffset - 1], verts[vertOffset], verts[vertOffset]);
							lastOnPoint = vecpts[1];
						}
					}
					else {
						// If there is no next command, then this is an 'end', so we should use the end marker if it exists
						drawMarker(ctx, groot, svgattr::marker_end(), MarkerPosition::MARKER_POSITION_END, vecpts[0], vecpts[1], vecpts[2]);
					}

					lastCmd = BL_PATH_CMD_ON;
					lastOnPoint = vecpts[1];
				}
				break;

				case BL_PATH_CMD_QUAD:
					nVerts = 2;
					//drawMarker(ctx, "marker-mid", MarkerPosition::MARKER_POSITION_MIDDLE, verts[vertOffset], verts[vertOffset], verts[vertOffset+1], groot);
					break;

				case BL_PATH_CMD_CONIC:
					//! Cubic-to control point (always used as a pair of commands).
					//printf("BL_PATH_CMD_CONIC\n");
					break;

				case BL_PATH_CMD_CUBIC: {
					nVerts = 3;
					vecpts[0] = verts[vertOffset + 1];
					vecpts[1] = verts[vertOffset + 2];
					vecpts[2] = verts[vertOffset + 2];

					if (cmdSpan.size() > 1)
					{
						uint8_t nextCmd = cmdSpan[1];
						switch (nextCmd) {
						case BL_PATH_CMD_ON:
						case BL_PATH_CMD_CUBIC:
							vecpts[2] = verts[vertOffset + nVerts];
							drawMarker(ctx, groot, svgattr::marker_mid(), MarkerPosition::MARKER_POSITION_MIDDLE, vecpts[0], vecpts[1], vecpts[2]);
							break;

						case BL_PATH_CMD_CLOSE:
							vecpts[2] = lastMoveTo;
							drawMarker(ctx, groot, svgattr::marker_mid(), MarkerPosition::MARKER_POSITION_MIDDLE, vecpts[0], vecpts[1], vecpts[2]);
							break;

						case BL_PATH_CMD_MOVE:
						default:
							vecpts[2] = vecpts[1];
							drawMarker(ctx, groot, svgattr::marker_end(), MarkerPosition::MARKER_POSITION_END, vecpts[0], vecpts[1], vecpts[2]);
						}
					}
					else {
						// If there is no next command, then this is an 'end', so we should use the end marker if it exists
						drawMarker(ctx, groot, svgattr::marker_end(), MarkerPosition::MARKER_POSITION_END, vecpts[0], vecpts[1], vecpts[2]);
					}

					lastCmd = BL_PATH_CMD_CUBIC;
					lastOnPoint = verts[vertOffset + 2];
				}
				break;

				case BL_PATH_CMD_CLOSE: {
					vecpts[0] = lastOnPoint;
					vecpts[1] = lastMoveTo;

					vecpts[2] = vecpts[1];
					if (numVerts > 1)
						vecpts[2] = verts[1];

					nVerts = 1;

					drawMarker(ctx, groot, svgattr::marker_end(), MarkerPosition::MARKER_POSITION_END, vecpts[0], vecpts[1], vecpts[2]);

					lastCmd = BL_PATH_CMD_CLOSE;
					lastOnPoint = vecpts[1];
				}
				break;

				}

				cmdSpan += nVerts;
				vertOffset += nVerts;
			}

			ctx->pop();
		}


		void drawSelf(IRenderSVG* ctx, IAmGroot* groot) override
		{
			PaintOrderProgram<uint8_t> prog(ctx->getPaintOrder());
			PathPainter painter{ctx, groot, fPath,
				// drawMarkersFn:
				fHasMarkers ? [](void* user, IRenderSVG* c, IAmGroot* g) {
					static_cast<SVGPathBasedGeometry*>(user)->drawMarkers(c, g);
				} : nullptr,
				// markersUser:
				this
			};

			painter.begin();
			prog.run(painter);
			painter.end();

			/*
			// Note:
			// Since we need to deal with markers, we can not
			// use the simple drawShape() function, as it does 
			// not deal with markers.
			ctx->beginDrawShape(fPath);

			// Get the paint order from the context
			// Note: It might be interesting to have a simple
			// functor that takes the porder as a 'program', and
			// the drawing routines as a lambda expression, or even
			// just a table of routines to be executed depending
			// on the instruction.
			uint32_t porder = ctx->getPaintOrder();

			for (int slot = 0; slot < 3; slot++)
			{
				uint32_t ins = porder & 0x03;	// get two lowest bits, which are a single instruction

				switch (ins)
				{
				case PaintOrderKind::SVG_PAINT_ORDER_FILL:
					ctx->fillShape(fPath);
					break;
					
				case PaintOrderKind::SVG_PAINT_ORDER_STROKE:
					ctx->strokeShape(fPath);
					break;

				case PaintOrderKind::SVG_PAINT_ORDER_MARKERS:
				{
					drawMarkers(ctx, groot);
				}
				break;
				}

				// discard instruction, shift down to get the next one ready
				porder = porder >> 2;
			}

			ctx->endDrawShape();
			*/
		}

	};
}

namespace waavs {
	
	//====================================
	// SVGLineElement
	//====================================
	struct SVGLineElement : public SVGPathBasedGeometry
	{
		static void registerFactory() {
			registerSVGSingularNode("line", [](IAmGroot* groot, const XmlElement& elem) {
				auto node = std::make_shared<SVGLineElement>(groot);
				node->loadFromXmlElement(elem, groot);
				return node;
			});
		}


		BLLine geom{};

		SVGLineElement(IAmGroot* iMap)
			:SVGPathBasedGeometry(iMap) {}

		/*
		BLRect getBBox() const override
		{
			// return a rectangle that represents the bounding box of the 
			// line geometry
			BLRect bounds{};
			bounds.x = waavs::min(geom.x0, geom.x1);
			bounds.y = waavs::min(geom.y0, geom.y1);
			bounds.w = waavs::max(geom.x0, geom.x1) - bounds.x;
			bounds.h = waavs::max(geom.y0, geom.y1) - bounds.y;
			
			return bounds;
		}
		*/

		
		void bindSelfToContext(IRenderSVG* ctx, IAmGroot* groot) override
		{	
			double dpi = 96;
			double w = 1.0;
			double h = 1.0;

			if (nullptr != groot)
			{
				dpi = groot->dpi();
			}


				BLRect cFrame = ctx->viewport();
				w = cFrame.w;
				h = cFrame.h;

			
			SVGDimension fDimX1{};
			SVGDimension fDimY1{};
			SVGDimension fDimX2{};
			SVGDimension fDimY2{};
			
			fDimX1.loadFromChunk(getAttributeByName("x1"));
			fDimY1.loadFromChunk(getAttributeByName("y1"));
			fDimX2.loadFromChunk(getAttributeByName("x2"));
			fDimY2.loadFromChunk(getAttributeByName("y2"));

			
			if (fDimX1.isSet())
				geom.x0 = fDimX1.calculatePixels(w, 0, dpi);
			if (fDimY1.isSet())
				geom.y0 = fDimY1.calculatePixels(h, 0, dpi);
			if (fDimX2.isSet())
				geom.x1 = fDimX2.calculatePixels(w, 0, dpi);
			if (fDimY2.isSet())
				geom.y1 = fDimY2.calculatePixels(h, 0, dpi);
			
			fPath.addLine(geom);
			fPath.shrink();
		}
		
	};
	

	//====================================
	// SVGRectElement
	//====================================
	struct SVGRectElement : public SVGPathBasedGeometry
	{
		static void registerSingular() {
			registerSVGSingularNode("rect", [](IAmGroot* groot, const XmlElement& elem) {
				auto node = std::make_shared<SVGRectElement>(groot);
				node->loadFromXmlElement(elem, groot);
				return node;
			});
		}
		
		static void registerFactory() {
			registerContainerNodeByName("rect",
				[](IAmGroot* groot, XmlPull& iter) {
					auto node = std::make_shared<SVGRectElement>(groot);
					node->loadFromXmlPull(iter, groot);

					return node;
				});
			

			registerSingular();
		}
		
		BLRoundRect geom{};
		bool fIsRound{ false };

		
		SVGRectElement(IAmGroot* iMap) :SVGPathBasedGeometry(iMap) {}
		
		BLRect getBBox() const override
		{
			return BLRect(geom.x, geom.y, geom.w, geom.h);
		}
		

		void bindSelfToContext(IRenderSVG* ctx, IAmGroot* groot) override
		{
			double dpi = 96;
			double w = 1.0;
			double h = 1.0;

			if (nullptr != groot)
			{
				dpi = groot->dpi();
			}
			

				BLRect cFrame = ctx->viewport();
				w = cFrame.w;
				h = cFrame.h;


			SVGDimension fX{};
			SVGDimension fY{};
			SVGDimension fWidth{};
			SVGDimension fHeight{};
			SVGDimension fRx{};
			SVGDimension fRy{};

			fX.loadFromChunk(getAttributeByName("x"));
			fY.loadFromChunk(getAttributeByName("y"));
			fWidth.loadFromChunk(getAttributeByName("width"));
			fHeight.loadFromChunk(getAttributeByName("height"));

			fRx.loadFromChunk(getAttributeByName("rx"));
			fRy.loadFromChunk(getAttributeByName("ry"));


			// If height or width <= 0, they are invalid
			//if (fWidth.value() < 0) { fWidth.fValue = 0; fWidth.fIsSet = false; }
			//if (fHeight.value() < 0) { fHeight.fValue = 0; fHeight.fIsSet = false; }

			// If radii are < 0, unset them
			//if (fRx.value() < 0) { fRx.fValue = 0; fRx.fIsSet = false; }
			//if (fRy.value() < 0) { fRy.fValue = 0; fRy.fIsSet = false; }

			
			
			geom.x = fX.calculatePixels(w, 0, dpi);
			geom.y = fY.calculatePixels(h, 0, dpi);
			geom.w = fWidth.calculatePixels(w, 0, dpi);
			geom.h = fHeight.calculatePixels(h, 0, dpi);

			if (fRx.isSet() || fRy.isSet())
			{
				fIsRound = true;
				if (fRx.isSet())
				{
					geom.rx = fRx.calculatePixels(w);
				}
				else
				{
					geom.rx = fRy.calculatePixels(h);
				}
				if (fRy.isSet())
				{
					geom.ry = fRy.calculatePixels(h);
				}
				else
				{
					geom.ry = fRx.calculatePixels(w);
				}

			}
			else if (fWidth.isSet() && fHeight.isSet())
			{
			}

			fPath.clear();
			if (fIsRound)
				fPath.addRoundRect(geom);
			else {
				fPath.addRect(geom.x, geom.y, geom.w, geom.h);
			}
			fPath.shrink();
		}
	
	};
	

	//====================================
	//  SVGCircleElement 
	//====================================
	struct SVGCircleElement : public SVGPathBasedGeometry
	{
		static void registerSingular() {
			registerSVGSingularNode("circle", [](IAmGroot* groot, const XmlElement& elem) {
				auto node = std::make_shared<SVGCircleElement>(groot);
				node->loadFromXmlElement(elem, groot);
				return node;
				});
		}

		static void registerFactory() {
			registerContainerNodeByName("circle",
				[](IAmGroot* groot, XmlPull& iter) {
					auto node = std::make_shared<SVGCircleElement>(groot);
					node->loadFromXmlPull(iter, groot);

					return node;
				});
			

			registerSingular();
		}

		
		BLCircle geom{};


		SVGCircleElement(IAmGroot* iMap) :SVGPathBasedGeometry(iMap) {}

		
		void bindSelfToContext(IRenderSVG *ctx, IAmGroot *groot) override
		{
			double dpi = 96;
			double w = 1.0;
			double h = 1.0;
			
			if (nullptr != groot)
			{
				dpi = groot->dpi();
			}
			

			BLRect cFrame = ctx->viewport();
				w = cFrame.w;
				h = cFrame.h;

			
			SVGDimension fCx{};
			SVGDimension fCy{};
			SVGDimension fR{};
			
			fCx.loadFromChunk(getAttributeByName("cx"));
			fCy.loadFromChunk(getAttributeByName("cy"));
			fR.loadFromChunk(getAttributeByName("r"));
			
			geom.cx = fCx.calculatePixels(w, 0, dpi);
			geom.cy = fCy.calculatePixels(h, 0, dpi);
			geom.r = fR.calculatePixels(w, h, dpi);

			fPath.clear();
			fPath.addCircle(geom);
			fPath.shrink();

		}
	};
	

	//====================================
	//	SVGEllipseElement 
	//====================================
	struct SVGEllipseElement : public SVGPathBasedGeometry
	{
		static void registerFactory() {
			registerSVGSingularNode("ellipse", [](IAmGroot* groot, const XmlElement& elem) {
				auto node = std::make_shared<SVGEllipseElement>(groot);
				node->loadFromXmlElement(elem, groot);
				
				return node; });
		}


		BLEllipse geom{};

		
		SVGEllipseElement(IAmGroot* iMap)
			:SVGPathBasedGeometry(iMap) {}
		
		
		BLRect getBBox() const override
		{
			return BLRect(geom.cx - geom.rx, geom.cy - geom.ry, geom.rx * 2, geom.ry * 2);
		}
		
		void bindSelfToContext(IRenderSVG* ctx, IAmGroot* groot) override
		{
			double dpi = 96;
			double w = 1.0;
			double h = 1.0;
			
			if (nullptr != groot)
			{
				dpi = groot->dpi();
			}
			

			BLRect cFrame = ctx->viewport();
			w = cFrame.w;
			h = cFrame.h;

			

			SVGDimension fCx{};
			SVGDimension fCy{};
			SVGDimension fRx{};
			SVGDimension fRy{};
			
			fCx.loadFromChunk(getAttributeByName("cx"));
			fCy.loadFromChunk(getAttributeByName("cy"));
			fRx.loadFromChunk(getAttributeByName("rx"));
			fRy.loadFromChunk(getAttributeByName("ry"));
			
			
			geom.cx = fCx.calculatePixels(w, 0, dpi);
			geom.cy = fCy.calculatePixels(h, 0, dpi);
			geom.rx = fRx.calculatePixels(w, 0, dpi);
			geom.ry = fRy.calculatePixels(h, 0, dpi);

			fPath.clear();
			fPath.addEllipse(geom);
			fPath.shrink();
		}

	};
	

	//====================================
	// SVGPolylineElement
	//
	//====================================
	struct SVGPolylineElement : public SVGPathBasedGeometry
	{
		static void registerFactory() {
			registerSVGSingularNode("polyline", [](IAmGroot* groot, const XmlElement& elem) {
				auto node = std::make_shared<SVGPolylineElement>(groot);
				node->loadFromXmlElement(elem, groot);
				
				return node;
			});
		}

		
		SVGPolylineElement(IAmGroot* iMap) :SVGPathBasedGeometry(iMap) {}
		
		void loadPoints(const ByteSpan& inChunk)
		{
			if (!inChunk)
				return;

			ByteSpan points = inChunk;

			double args[2]{ 0 };


			if (readNumericArguments(points, "cc", args))
			{
				fPath.moveTo(args[0], args[1]);

				while (points)
				{
					if (!readNumericArguments(points, "cc", args))
						break;

					fPath.lineTo(args[0], args[1]);
				}
			}
		}
		
		void bindSelfToContext(IRenderSVG* ctx, IAmGroot* groot) override
		{
			fPath.clear();

			loadPoints(getAttributeByName("points"));
			fPath.shrink();
		}
	};
	

	//====================================
	//
	// SVGPolygonElement
	//
	//====================================
	struct SVGPolygonElement : public SVGPolylineElement
	{
		static void registerSingularNode()
		{
			registerSVGSingularNode("polygon", [](IAmGroot* groot, const XmlElement& elem) {
				auto node = std::make_shared<SVGPolygonElement>(groot);
				node->loadFromXmlElement(elem, groot);
				return node;
				});
		}

		static void registerFactory() {
			registerContainerNodeByName("polygon",
				[](IAmGroot* groot, XmlPull& iter) {
					auto node = std::make_shared<SVGPolygonElement>(groot);
					node->loadFromXmlPull(iter, groot);

					return node;
				});
			

			registerSingularNode();
		}

		SVGPolygonElement(IAmGroot* iMap) 
			:SVGPolylineElement(iMap) {}

		void bindSelfToContext(IRenderSVG* ctx, IAmGroot* groot) override
		{
			SVGPolylineElement::bindSelfToContext(ctx, groot);
			
			fPath.close();
			fPath.shrink();
		}
	};
	

	//====================================
	// SVGPath
	// <path>
	//====================================
	struct SVGPathElement : public SVGPathBasedGeometry
	{
		static void registerSingularNode() {
			registerSVGSingularNode("path", [](IAmGroot* groot, const XmlElement& elem) {
				auto node = std::make_shared<SVGPathElement>(groot);
				node->loadFromXmlElement(elem, groot);
				
				return node;
			});
		}

		static void registerFactory()
		{
			registerContainerNodeByName("path",
				[](IAmGroot* groot, XmlPull& iter) {
					auto node = std::make_shared<SVGPathElement>(groot);
					node->loadFromXmlPull(iter, groot);

					return node;
				});
			

			registerSingularNode();
		}
		
		SVGPathElement(IAmGroot* iMap) 
			:SVGPathBasedGeometry(iMap)
		{
		}
		
		virtual void bindSelfToContext(IRenderSVG*, IAmGroot*) override
		{
			fPath.clear();

			auto d = getAttribute(svgattr::d());
			if (d) {
				parsePath(d, fPath);
				fPath.shrink();
			}
		}


	};

}
