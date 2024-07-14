#pragma once

#include <string>
#include <array>
#include <functional>
#include <unordered_map>

#include "svgattributes.h"
#include "svgpath.h"
#include "svgtext.h"
#include "viewport.h"


namespace waavs {
	//=================================================
	// SVGMarkerNode
	// Reference: https://svg-art.ru/?page_id=855
	//=================================================
	enum MarkerUnits {
		UserSpaceOnUse = 0,
		StrokeWidth = 1
	};

	struct SVGMarkerNode : public SVGGraphicsElement
	{
		static void registerFactory()
		{
			gSVGGraphicsElementCreation["marker"] = [](IAmGroot* aroot, XmlElementIterator& iter) {
				auto node = std::make_shared<SVGMarkerNode>(aroot);
				node->loadFromXmlIterator(iter);
				return node;
			};
		}

		// Fields
		SVGViewbox fViewbox{};
		SVGDimension fDimRefX{};
		SVGDimension fDimRefY{};
		SVGDimension fDimMarkerWidth{};
		SVGDimension fDimMarkerHeight{};
		uint32_t fMarkerUnits{StrokeWidth};
		SVGOrient fOrientation{ nullptr };

		// Resolved field values
		double fRefX = 0;
		double fRefY = 0;
		double fMarkerWidth{ 3 };
		double fMarkerHeight{ 3 };
		
		SVGMarkerNode(IAmGroot* root) 
			: SVGGraphicsElement(root) 
		{
			isStructural(false);
		}

		const SVGOrient& orientation() const { return fOrientation; }

		
		void bindSelfToGroot(IAmGroot* groot) override
		{
			
			if (fDimMarkerWidth.isSet())
				fMarkerWidth = fDimMarkerWidth.calculatePixels();
			else if (fViewbox.isSet())
				fMarkerWidth = fViewbox.width();
			
			if (fDimMarkerHeight.isSet())
				fMarkerHeight = fDimMarkerHeight.calculatePixels();
			else if (fViewbox.isSet())
				fMarkerHeight = fViewbox.height();
			
			if (fDimRefX.isSet())
				fRefX = fDimRefX.calculatePixels();
			else if (fViewbox.isSet())
				fRefX = fViewbox.x();
		}
		
		void applyAttributes(IRenderSVG* ctx) override
		{
			SVGGraphicsElement::applyAttributes(ctx);

			double sWidth = ctx->strokeWidth();
			double scaleX = 1.0;
			double scaleY = 1.0;
			
			if (fMarkerUnits == StrokeWidth)
			{
				// scale == strokeWidth
				scaleX = sWidth;
				scaleY = sWidth;
				ctx->scale(scaleX, scaleY);
			}
			else
			{
				if (fViewbox.isSet())
				{
					scaleX = fMarkerWidth / fViewbox.width();
					scaleY = fMarkerHeight / fViewbox.height();
					ctx->scale(scaleX, scaleY);
				}
				// No scaling
			}

			
			ctx->translate(-fDimRefX.calculatePixels(), -fDimRefY.calculatePixels());
		}

		void drawChildren(IRenderSVG* ctx) override
		{
			ctx->push();
			
			// Setup drawing state
			ctx->blendMode(BL_COMP_OP_SRC_OVER);
			ctx->fill(BLRgba32(0, 0, 0));
			ctx->noStroke();
			ctx->strokeWidth(1.0);
			ctx->setStrokeJoin(BLStrokeJoin::BL_STROKE_JOIN_MITER_BEVEL);
			
			SVGGraphicsElement::drawChildren(ctx);
			
			ctx->pop();
		}
		
		void loadVisualProperties(const XmlAttributeCollection& attrs) override
		{
			SVGGraphicsElement::loadVisualProperties(attrs);
			
			auto preserveAspectRatio = attrs.getAttribute("preserveAspectRatio");

			
			if (attrs.getAttribute("viewBox")) {
				fViewbox.loadFromChunk(attrs.getAttribute("viewBox"));
			}

			if (attrs.getAttribute("refX")) {
				fDimRefX.loadFromChunk(attrs.getAttribute("refX"));
			}
			if (attrs.getAttribute("refY")) {
				fDimRefY.loadFromChunk(attrs.getAttribute("refY"));
			}
			
			if (attrs.getAttribute("markerWidth")) {
				fDimMarkerWidth.loadFromChunk(attrs.getAttribute("markerWidth"));
			}
			
			if (attrs.getAttribute("markerHeight")) {
				fDimMarkerHeight.loadFromChunk(attrs.getAttribute("markerHeight"));
			}

			if (attrs.getAttribute("markerUnits")) {
				auto units = attrs.getAttribute("markerUnits");
				if (units == "strokeWidth") {
					fMarkerUnits = StrokeWidth;
				}
				else if (units == "userSpaceOnUse") {
					fMarkerUnits = UserSpaceOnUse;
				}
			}

			if (attrs.getAttribute("orient")) {
				fOrientation.loadFromChunk(attrs.getAttribute("orient"));
			}

		}


	};

	
	//===================================================
	// SVGGeometryElement
	// All geometries are represented as a path.  This base
	// class is all that is needed to represent a path
	//===================================================
	
	struct SVGGeometryElement : public SVGGraphicsElement
	{
		BLPath fPath{};
		BLPath fStrokedPath{};
		bool fIsStroked{ false };
		bool fHasMarkers{ false };
		
		SVGGeometryElement(IAmGroot* iMap) :SVGGraphicsElement(iMap) {}
		
		// Return bounding rectangle for shape
		// This does not include the stroke width
		BLRect frame() const override
		{
			BLBox bbox{};
			fPath.getBoundingBox(&bbox);
			if (fHasTransform) {
				auto leftTop = fTransform.mapPoint(bbox.x0, bbox.y0);
				auto rightBottom = fTransform.mapPoint(bbox.x1, bbox.y1);
				return BLRect(leftTop.x, leftTop.y, rightBottom.x - leftTop.x, rightBottom.y - leftTop.y);
			}

			return BLRect(bbox.x0, bbox.y0, bbox.x1 - bbox.x0, bbox.y1 - bbox.y0);
		}
		
		BLRect getBBox() const override
		{
			BLBox bbox{};
			fPath.getBoundingBox(&bbox);

			// if we have markers turned on, add in the bounding
			// box of the markers
			
			if (fHasTransform) {
				auto leftTop = fTransform.mapPoint(bbox.x0, bbox.y0);
				auto rightBottom = fTransform.mapPoint(bbox.x1, bbox.y1);
				return BLRect(leftTop.x, leftTop.y, (rightBottom.x - leftTop.x), (rightBottom.y - leftTop.y));
			}

			return BLRect(bbox.x0, bbox.y0, (bbox.x1 - bbox.x0), (bbox.y1 - bbox.y0));
		}
		
		bool contains(double x, double y) override
		{
			BLPoint localPoint(x, y);
			
			// check to see if we have a transform property
			// if we do, transform the points through that transform
			// then check to see if the point is inside the transformed path
			if (fHasTransform)
			{
				// get the inverse transform
				auto inverse = fTransform;
				inverse.invert();
				localPoint = inverse.mapPoint(localPoint);
			}

		
			
			// BUGBUG - should use the fill-rule attribute that will actually apply
			BLHitTest ahit = fPath.hitTest(localPoint, BLFillRule::BL_FILL_RULE_EVEN_ODD);
			return (ahit == BLHitTest::BL_HIT_TEST_IN);
		}

		void bindPropertiesToGroot(IAmGroot* groot) override
		{
			SVGGraphicsElement::bindPropertiesToGroot(groot);
			
			if (((fVisualProperties.find("marker-start") != fVisualProperties.end()) && fVisualProperties["marker-start"]->isSet()) ||
				((fVisualProperties.find("marker-mid") != fVisualProperties.end()) && fVisualProperties["marker-mid"]->isSet()) ||
				((fVisualProperties.find("marker-end") != fVisualProperties.end()) && fVisualProperties["marker-end"]->isSet()) ||
				((fVisualProperties.find("marker") != fVisualProperties.end()) && fVisualProperties["marker"]->isSet()))
			{
				fHasMarkers = true;
			}
		}
		
		bool drawMarker(IRenderSVG* ctx, std::shared_ptr<SVGVisualProperty> prop, MarkerPosition pos, const BLPoint &p1, const BLPoint &p2)
		{
			// cast the prop to a SVGMarkerAttribute
			auto marker = std::dynamic_pointer_cast<SVGMarkerAttribute>(prop);

			if (nullptr == marker)
				return false;

			// get the marker node, as shared_ptr<SVGViewable>
			auto aNode = marker->markerNode();
			if (nullptr == aNode)
				return false;

			// cast the node to a SVGMarkerNode
			std::shared_ptr<SVGMarkerNode> markerNode = std::dynamic_pointer_cast<SVGMarkerNode>(aNode);
			
			
			ctx->push();
			
			BLPoint transP{};
			
			switch (pos)
			{
			case MarkerPosition::MARKER_POSITION_START:
				transP = p1;
				break;
			case MarkerPosition::MARKER_POSITION_MIDDLE:
				transP = p2;
				break;
			case MarkerPosition::MARKER_POSITION_END:
				transP = p2;
				break;
			}

			// Use the two given points to calculate the angle of rotation 
			// from the markerNode
			double rads = markerNode->orientation().calculateRadians(pos, p1, p2);

			// apply the transformation
			ctx->translate(transP);
			ctx->rotate(rads);
			
			// draw the marker
			markerNode->draw(ctx);

				
			ctx->pop();

			return true;
		}
		
		/*
		// for debugging purposes
		// draw the vertices of the path
		void drawVertices(IRenderSVG* ctx)
		{	
			for (int i = 0; i < fPath.size(); i++)
			{
				BLPoint pt = fPath.vertexData()[i];
				ctx->fillCircle(pt.x, pt.y, 5);
				ctx->strokeCircle(pt.x, pt.y, 5);
			}
		}
		*/
		
		// traverse the points of the path
		// drawing a marker at each point
		void drawMarkers(IRenderSVG* ctx)
		{
			// get general marker if it exists
			auto marker = getVisualProperty("marker");
			if (marker!=nullptr && marker->needsBinding())
				marker->bindToGroot(root());
			
			// draw first marker
			auto markerStart = getVisualProperty("marker-start");
			if (markerStart != nullptr && markerStart->isSet() && fPath.size() >=2)
			{
				// do the binding if necessary
				if (markerStart->needsBinding())
					markerStart->bindToGroot(root());

				// get the first couple of points
				// and draw the single marker
				BLPoint p1 = fPath.vertexData()[0];
				BLPoint p2 = fPath.vertexData()[1];
				
				drawMarker(ctx, markerStart, MarkerPosition::MARKER_POSITION_START, p1, p2);
			}

			
			// draw mid-markers
			auto markerMid = getVisualProperty("marker-mid");

			if (nullptr != markerMid && markerMid->isSet() && fPath.size() >= 2)
			{
				// do the binding if necessary
				if (markerMid->needsBinding())
					markerMid->bindToGroot(root());

				for (int i = 1; i < fPath.size() - 1; i++)
				{
					// Get a couple of points
					BLPoint p1 = fPath.vertexData()[i - 1];
					BLPoint p2 = fPath.vertexData()[i];

					drawMarker(ctx, markerMid, MarkerPosition::MARKER_POSITION_MIDDLE, p1, p2);
				}
			}
			
			// draw last marker
			auto markerEnd = getVisualProperty("marker-end");

			if (nullptr != markerEnd && markerEnd->isSet() && fPath.size() >= 2)
			{
				// do the binding if necessary
				if (markerEnd->needsBinding())
					markerEnd->bindToGroot(root());

				// Get the last two points
				BLPoint p1 = fPath.vertexData()[fPath.size() - 2];
				BLPoint p2 = fPath.vertexData()[fPath.size() - 1];
				
				drawMarker(ctx, markerEnd, MarkerPosition::MARKER_POSITION_END, p1, p2);
			}
			
		}
		
		void drawSelf(IRenderSVG *ctx) override
		{
			// The paint-order attribute can change which
			// order these are done in
			//ctx->path(fPath);
			//printf("SVGGeometryElement::drawSelf(%s)\n", id().c_str());
			
			ctx->fillPath(fPath);
			ctx->strokePath(fPath);
			//ctx->flush();
			
			// draw markers if we have any
			if (fHasMarkers)
				drawMarkers(ctx);
		}
		
	};
	


	struct SVGLineElement : public SVGGeometryElement
	{
		static void registerFactory() {
			gShapeCreationMap["line"] = [](IAmGroot* root, const XmlElement& elem) {
				auto node = std::make_shared<SVGLineElement>(root);
				node->loadFromXmlElement(elem);
				return node;
			};
		}


		
		SVGDimension fDimX1{};
		SVGDimension fDimY1{};
		SVGDimension fDimX2{};
		SVGDimension fDimY2{};
		
		
		SVGLineElement(IAmGroot* iMap)
			:SVGGeometryElement(iMap) {}
		
	
		void bindSelfToGroot(IAmGroot* groot) override
		{	
			BLLine geom{};
			double dpi = 96;
			double w = 1.0;
			double h = 1.0;

			if (nullptr != groot)
			{
				dpi = groot->dpi();
				w = groot->canvasWidth();
				h = groot->canvasHeight();
			}
			
			double x1 = 0;
			double y1 = 0;
			double x2 = 0;
			double y2 = 0;
			
			if (fDimX1.isSet())
				x1 = fDimX1.calculatePixels(w, 0, dpi);
			if (fDimY1.isSet())
				y1 = fDimY1.calculatePixels(h, 0, dpi);
			if (fDimX2.isSet())
				x2 = fDimX2.calculatePixels(w, 0, dpi);
			if (fDimY2.isSet())
				y2 = fDimY2.calculatePixels(h, 0, dpi);
			
			geom.x0 = x1;
			geom.y0 = y1;
			geom.x1 = x2;
			geom.y1 = y2;
			
			fPath.addLine(geom);
		}
		
		void loadVisualProperties(const XmlAttributeCollection& attrs) override
		{
			SVGGeometryElement::loadVisualProperties(attrs);

			fDimX1.loadFromChunk(getAttribute("x1"));
			fDimY1.loadFromChunk(getAttribute("y1"));
			fDimX2.loadFromChunk(getAttribute("x2"));
			fDimY2.loadFromChunk(getAttribute("y2"));

			needsBinding(true);
		}
		
	};
	
	struct SVGRectElement : public SVGGeometryElement
	{
		static void registerSingular() {
			gShapeCreationMap["rect"] = [](IAmGroot* root, const XmlElement& elem) {
				auto node = std::make_shared<SVGRectElement>(root);
				node->loadFromXmlElement(elem);
				return node;
			};
		}
		
		static void registerFactory() {
			gSVGGraphicsElementCreation["rect"] = [](IAmGroot* aroot, XmlElementIterator& iter) {
				auto node = std::make_shared<SVGRectElement>(aroot);
				node->loadFromXmlIterator(iter);
				return node;
				};

			registerSingular();
		}
		
		
		SVGDimension fX{};
		SVGDimension fY{};
		SVGDimension fWidth{};
		SVGDimension fHeight{};
		SVGDimension fRx{};
		SVGDimension fRy{};
		
		SVGRectElement(IAmGroot* iMap) :SVGGeometryElement(iMap) {}
		
		void bindSelfToGroot(IAmGroot* groot) override
		{
			
			BLRoundRect geom{};
			
			double dpi = 96;
			double w = 1.0;
			double h = 1.0;

			if (groot)
			{
				dpi = groot->dpi();
				w = groot->canvasWidth();
				h = groot->canvasHeight();
			}

			// If height or width < 0, they are invalid
			//if (fWidth.value() < 0) { fWidth.fValue = 0; fWidth.fIsSet = false; }
			//if (fHeight.value() < 0) { fHeight.fValue = 0; fHeight.fIsSet = false; }

			// If radii are < 0, unset them
			//if (fRx.value() < 0) { fRx.fValue = 0; fRx.fIsSet = false; }
			//if (fRy.value() < 0) { fRy.fValue = 0; fRy.fIsSet = false; }

			
			
			geom.x = fX.calculatePixels(w);
			geom.y = fY.calculatePixels(h);
			geom.w = fWidth.calculatePixels(w);
			geom.h = fHeight.calculatePixels(h);

			if (fRx.isSet() || fRy.isSet())
			{
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
				fPath.addRoundRect(geom);
			}
			else if (fWidth.isSet() && fHeight.isSet())
			{
				fPath.addRect(geom.x, geom.y, geom.w, geom.h);
			}

			// BUGBUG - turn off fill-rule attribute if it exists
			// Because it makes something not display?
			// what was it that was subject to this?
			// poker-13-rey-de-diamantes-vectorizado.svg
			// if you comment this out, then the red diamonds will not
			// be filled in.
			//auto fillRule = getVisualProperty("fill-rule");
			//if (fillRule)
			//{
			//	fillRule->autoDraw(false);
			//}
		}
		
		void loadVisualProperties(const XmlAttributeCollection& attrs) override
		{
			SVGGeometryElement::loadVisualProperties(attrs);

			if (attrs.getAttribute("x"))
				fX.loadFromChunk(attrs.getAttribute("x"));
			if (attrs.getAttribute("y"))
				fY.loadFromChunk(attrs.getAttribute("y"));
			if (attrs.getAttribute("width"))
				fWidth.loadFromChunk(attrs.getAttribute("width"));
			if (attrs.getAttribute("height"))
				fHeight.loadFromChunk(attrs.getAttribute("height"));
			if (attrs.getAttribute("rx"))
				fRx.loadFromChunk(attrs.getAttribute("rx"));
			if (attrs.getAttribute("ry"))
				fRy.loadFromChunk(attrs.getAttribute("ry"));
			
			needsBinding(true);
		}
		
	};
	
	struct SVGCircleElement : public SVGGeometryElement
	{
		static void registerSingular() {
			gShapeCreationMap["circle"] = [](IAmGroot* root, const XmlElement& elem) {
				auto node = std::make_shared<SVGCircleElement>(root);
				node->loadFromXmlElement(elem);
				return node;
				};
		}

		static void registerFactory() {
			gSVGGraphicsElementCreation["circle"] = [](IAmGroot* aroot, XmlElementIterator& iter) {
				auto node = std::make_shared<SVGCircleElement>(aroot);
				node->loadFromXmlIterator(iter);
				return node;
				};

			registerSingular();
		}

		
		
		SVGDimension fCx{};
		SVGDimension fCy{};
		SVGDimension fR{};

		
		SVGCircleElement(IAmGroot* iMap) :SVGGeometryElement(iMap) {}

		void bindSelfToGroot(IAmGroot *groot) override
		{

			BLCircle geom{};
			double dpi = 96;
			double w = 1.0;
			double h = 1.0;
			
			if (nullptr != groot)
			{
				dpi = groot->dpi();
				w = groot->canvasWidth();
				h = groot->canvasHeight();
			}
			
			geom.cx = fCx.calculatePixels(w, 0, dpi);
			geom.cy = fCy.calculatePixels(h, 0, dpi);
			geom.r = fR.calculatePixels(w, h, dpi);

			fPath.addCircle(geom);
		}
		
		void loadVisualProperties(const XmlAttributeCollection& attrs) override
		{
			SVGGeometryElement::loadVisualProperties(attrs);
		
			if (attrs.getAttribute("cx"))
				fCx.loadFromChunk(attrs.getAttribute("cx"));
			if (attrs.getAttribute("cy"))
				fCy.loadFromChunk(attrs.getAttribute("cy"));
			if (attrs.getAttribute("r"))
				fR.loadFromChunk(attrs.getAttribute("r"));
			
			needsBinding(true);
		}
		

	};
	
	struct SVGEllipseElement : public SVGGeometryElement
	{
		static void registerFactory() {
			gShapeCreationMap["ellipse"] = [](IAmGroot* root, const XmlElement& elem) {
				auto node = std::make_shared<SVGEllipseElement>(root);
				node->loadFromXmlElement(elem);
				return node;
			};
		}

		SVGDimension fCx{};
		SVGDimension fCy{};
		SVGDimension fRx{};
		SVGDimension fRy{};
		
		SVGEllipseElement(IAmGroot* iMap)
			:SVGGeometryElement(iMap) {}
		

		void bindSelfToGroot(IAmGroot* groot) override
		{

			double dpi = 96;
			double w = 1.0;
			double h = 1.0;
			
			if (nullptr != groot)
			{
				dpi = groot->dpi();
				w = groot->canvasWidth();
				h = groot->canvasHeight();
			}
			
			BLEllipse geom{};

			geom.cx = fCx.calculatePixels(w, 0, dpi);
			geom.cy = fCy.calculatePixels(h, 0, dpi);
			geom.rx = fRx.calculatePixels(w, 0, dpi);
			geom.ry = fRy.calculatePixels(h, 0, dpi);

			fPath.addEllipse(geom);

		}

		void loadVisualProperties(const XmlAttributeCollection& attrs) override
		{
			SVGGeometryElement::loadVisualProperties(attrs);
			
			if (attrs.getAttribute("cx"))
				fCx.loadFromChunk(attrs.getAttribute("cx"));
			if (attrs.getAttribute("cy"))
				fCy.loadFromChunk(attrs.getAttribute("cy"));
			if (attrs.getAttribute("rx"))
				fRx.loadFromChunk(attrs.getAttribute("rx"));
			if (attrs.getAttribute("ry"))
				fRy.loadFromChunk(attrs.getAttribute("ry"));
			
			needsBinding(true);
		}
		

	};
	
	struct SVGPolylineElement : public SVGGeometryElement
	{
		static void registerFactory() {
			gShapeCreationMap["polyline"] = [](IAmGroot* root, const XmlElement& elem) {
				auto node = std::make_shared<SVGPolylineElement>(root);
				node->loadFromXmlElement(elem);
				return node;
			};
		}

		
		SVGPolylineElement(IAmGroot* iMap) :SVGGeometryElement(iMap) {}
		
		void loadPoints(const ByteSpan &pts)
		{
			if (!pts)
				return;
			
			ByteSpan points = pts;
			
			// Assume there's at least one point
			// if not, then we should error out
			// and mark this as not visible
			BLPoint pt{};
			if (!parseNextNumber(points, pt.x))
				return;
			if (!parseNextNumber(points, pt.y))
				return;

			fPath.moveTo(pt);

			while (points)
			{
				if (!parseNextNumber(points, pt.x))
					break;
				if (!parseNextNumber(points, pt.y))
					break;

				fPath.lineTo(pt);
			}

			needsBinding(true);
		}
		
		void loadVisualProperties(const XmlAttributeCollection& attrs) override
		{
			SVGGeometryElement::loadVisualProperties(attrs);

			if (attrs.getAttribute("points"))
			{
				ByteSpan points = attrs.getAttribute("points");
				loadPoints(points);
			}

		}

	};
	

	
	struct SVGPolygonElement : public SVGGeometryElement
	{
		static void registerSingularNode()
		{
			gShapeCreationMap["polygon"] = [](IAmGroot* root, const XmlElement& elem) {
				auto node = std::make_shared<SVGPolygonElement>(root);
				node->loadFromXmlElement(elem);
				return node;
				};
		}

		static void registerFactory() {
			gSVGGraphicsElementCreation["polygon"] = [](IAmGroot* aroot, XmlElementIterator& iter) {
				auto node = std::make_shared<SVGPolygonElement>(aroot);
				node->loadFromXmlIterator(iter);
				return node;
				};

			registerSingularNode();
		}

		
		SVGPolygonElement(IAmGroot* iMap) :SVGGeometryElement(iMap) {}
		
		void loadVisualProperties(const XmlAttributeCollection& attrs) override
		{
			SVGGeometryElement::loadVisualProperties(attrs);

			auto points = getAttribute("points");

			BLPoint pt{};
			parseNextNumber(points, pt.x);
			parseNextNumber(points, pt.y);


			fPath.moveTo(pt);

			while (points)
			{
				if (!parseNextNumber(points, pt.x))
					break;
				if (!parseNextNumber(points, pt.y))
					break;

				fPath.lineTo(pt);
			}
			fPath.close();

			needsBinding(true);
		}
		
	};
	

	//====================================
	// SVGPath
	// <path>
	//====================================
	
	struct SVGPathElement : public SVGGeometryElement
	{
		static void registerSingularNode() {
			gShapeCreationMap["path"] = [](IAmGroot* root, const XmlElement& elem) {
				auto node = std::make_shared<SVGPathElement>(root);
				node->loadFromXmlElement(elem);
				return node;
			};
		}

		static void registerFactory()
		{
			gSVGGraphicsElementCreation["path"] = [](IAmGroot* aroot, XmlElementIterator& iter) {
				auto node = std::make_shared<SVGPathElement>(aroot);
				node->loadFromXmlIterator(iter);
				return node;
			};

			registerSingularNode();
		}
		
		SVGPathElement(IAmGroot* iMap) :SVGGeometryElement(iMap) 
		{
			//fUseCacheIsolation = true;
		}
		
		/*
		void loadVisualProperties(const XmlAttributeCollection& attrs) override
		{
			SVGGeometryElement::loadVisualProperties(attrs);
		
			auto d = getAttribute("d");
			if (!d)
				return;

			auto success = blpathparser::parsePath(d, fPath);
			fPath.shrink();

			needsBinding(true);

		}
		*/
		
		///*
		void loadSelfFromXmlElement(const XmlElement& elem) override
		{	
			auto d = getAttribute("d");
			if (!d)
				return;
			
			auto success = blpathparser::parsePath(d, fPath);
			if (!success)
			{
				printf("loadSelfFromXmlElement - failed parsePath: %d\n", success);
			}
			
			fPath.shrink();
			
			needsBinding(true);
		}
		//*/
	};


	
	//====================================
	// SVGUseNode
	// <use>
	//====================================
	struct SVGUseElement : public SVGGraphicsElement
	{
		static void registerSingularNode()
		{
			gShapeCreationMap["use"] = [](IAmGroot* aroot, const XmlElement& elem) {
				auto node = std::make_shared<SVGUseElement>(aroot);
				node->loadFromXmlElement(elem);
				return node;
			};
		}

		static void registerFactory()
		{
			gSVGGraphicsElementCreation["use"] = [](IAmGroot* aroot, XmlElementIterator& iter) {
				auto node = std::make_shared<SVGUseElement>(aroot);
				node->loadFromXmlIterator(iter);
				return node;
			};

			registerSingularNode();
		}

		
		std::shared_ptr<SVGViewable> fWrappedNode{nullptr};
		std::string fWrappedID{};

		double x{ 0 };
		double y{ 0 };
		double width{ 0 };
		double height{ 0 };
		
		SVGDimension fX{};
		SVGDimension fY{};
		SVGDimension fWidth{};
		SVGDimension fHeight{};


		SVGUseElement(IAmGroot* aroot) : SVGGraphicsElement(aroot) {}

		const BLVar& getVariant() override
		{
			if (fWrappedNode)
				return fWrappedNode->getVariant();
			else
				return SVGGraphicsElement::getVariant();
		}

		BLRect frame() const override
		{
			// BUGBUG: This is not correct
			// Needs to be adjusted for the x/y
			// and the transform
			if (fWrappedNode != nullptr)
				return fWrappedNode->frame();

			return BLRect{ };
		}
		
		void bindSelfToGroot(IAmGroot* groot) override
		{
			
			double dpi = 96;
			double w = 1.0;
			double h = 1.0;

			if (nullptr != groot)
			{
				dpi = groot->dpi();
				w = groot->canvasWidth();
				h = groot->canvasHeight();
			}


			if (fX.isSet())
				x = fX.calculatePixels(w,0,dpi);
			if (fY.isSet())
				y = fY.calculatePixels(h, 0, dpi);
			if (fWidth.isSet())
				width = fWidth.calculatePixels(w, 0, dpi);
			if (fHeight.isSet())
				height = fHeight.calculatePixels(h, 0, dpi);
			
			
			// Use the root to lookup the wrapped node
			if (groot != nullptr && !fWrappedID.empty())
			{
				fWrappedNode = groot->getElementById(fWrappedID);

				if (fWrappedNode)
				{
					fWrappedNode->bindToGroot(groot);
				}
			}
			
			needsBinding(false);
		}

		// Apply locally generated attributes
		void applyAttributes(IRenderSVG* ctx) override
		{
			SVGGraphicsElement::applyAttributes(ctx);
			
			// perform transform if we have one

			ctx->translate(x, y);
			
			// set local size, if width and height were set
			// we don't want to do scaling here, because the
			// wrapped graphic might want to do something different
			// really it applies to symbols, and they're do their 
			// own scaling.
			if (fWidth.isSet() && fHeight.isSet())
			{
				ctx->localSize(width, height);
			}

		}

		void loadVisualProperties(const XmlAttributeCollection& attrs) override
		{
			SVGGraphicsElement::loadVisualProperties(attrs);


			// look for the href, or xlink:href attribute
			auto href = attrs.getAttribute("href");
			if (!href)
			{
				href = chunk_trim(attrs.getAttribute("xlink:href"), xmlwsp);
			}

			// Capture the href to be looked up later
			if (href && *href == '#')
			{
				href++;
				fWrappedID = toString(href);
			}
			
			if (attrs.getAttribute("x"))
				fX.loadFromChunk(attrs.getAttribute("x"));
			if (attrs.getAttribute("y"))
				fY.loadFromChunk(attrs.getAttribute("y"));
			if (attrs.getAttribute("width"))
				fWidth.loadFromChunk(attrs.getAttribute("width"));
			if (attrs.getAttribute("height"))
				fHeight.loadFromChunk(attrs.getAttribute("height"));

			needsBinding(true);
		}
		
		void drawSelf(IRenderSVG* ctx) override
		{
			// Draw the wrapped graphic
			if (fWrappedNode != nullptr)
				fWrappedNode->draw(ctx);
		}



		
		//void loadSelfFromXmlElement(const XmlElement& elem) override
		//{
		//	loadVisualProperties(*this);
		//}
		
	};

	//==================================================
	// SVGImageNode
	// Stores embedded or referenced images
	//==================================================

	struct SVGImageNode : public SVGGraphicsElement
	{
		static void registerSingularNode()
		{
			gShapeCreationMap["image"] = [](IAmGroot* aroot, const XmlElement& elem) {
				auto node = std::make_shared<SVGImageNode>(aroot);
				node->loadFromXmlElement(elem);
				return node;
			};
		}

		static void registerFactory()
		{
			gSVGGraphicsElementCreation["image"] = [](IAmGroot* aroot, XmlElementIterator& iter) {
				auto node = std::make_shared<SVGImageNode>(aroot);
				node->loadFromXmlIterator(iter);
				return node;
			};

			registerSingularNode();
		}

		
		BLImage fImage{};
		ByteSpan fImageRef;
		
		double fX{ 0 };
		double fY{ 0 };
		double fWidth{ 0 };
		double fHeight{ 0 };
		
		SVGDimension fDimX{};
		SVGDimension fDimY{};
		SVGDimension fDimWidth{};
		SVGDimension fDimHeight{};


		SVGImageNode(IAmGroot* root) 
			: SVGGraphicsElement(root) {}

		BLRect frame() const override
		{

			if (fHasTransform) {
				auto leftTop = fTransform.mapPoint(fX, fY);
				auto rightBottom = fTransform.mapPoint(fX + fWidth, fY + fHeight);
				return BLRect(leftTop.x, leftTop.y, rightBottom.x - leftTop.x, rightBottom.y - leftTop.y);
			}

			return BLRect(fX, fY, fWidth,fHeight);
		}
		
		const BLVar& getVariant() override
		{
			if (fVar.isNull())
			{
				fVar.assign(fImage);
			}

			return fVar;
		}

		void bindSelfToGroot(IAmGroot* groot) override
		{
			double dpi = 96;
			double w = 1.0;
			double h = 1.0;

			if (nullptr != groot)
			{
				dpi = groot->dpi();
				w = groot->canvasWidth();
				h = groot->canvasHeight();
			}

			// Parse the image so we can get its dimensions
			if (fImageRef)
			{
				// First, see if it's embedded data
				if (chunk_starts_with_cstr(fImageRef, "data:"))
				{
					bool success = parseImage(fImageRef, fImage);
					//printf("SVGImageNode::fImageRef, parseImage: %d\n", success);
				}
				else {
					// Otherwise, assume it's a file reference
					auto path = toString(fImageRef);
					if (path.size() > 0)
					{
						fImage.readFromFile(path.c_str());
					}
				}
			}
			
			// BUGBUG - sanity check of image data
			BLImageData imgData{};
			
			fImage.getData(&imgData);
			
			//printf("imgData: %d x %d  depth: %d\n", imgData.size.w, imgData.size.h, fImage.depth());
			
			fX = 0;
			fY = 0;
			fWidth = fImage.size().w;
			fHeight = fImage.size().h;
			
			if (fDimX.isSet())
				fX = fDimX.calculatePixels(w, 0, dpi);
			if (fDimY.isSet())
				fY = fDimY.calculatePixels(h, 0, dpi);
			if (fDimWidth.isSet())
				fWidth = fDimWidth.calculatePixels(w, 0, dpi);
			if (fDimHeight.isSet())
				fHeight = fDimHeight.calculatePixels(h, 0, dpi);
			

		}

		virtual void loadVisualProperties(const XmlAttributeCollection& attrs)
		{
			SVGGraphicsElement::loadVisualProperties(attrs);

			if (attrs.getAttribute("x"))
				fDimX.loadFromChunk(attrs.getAttribute("x"));
			if (attrs.getAttribute("y"))
				fDimY.loadFromChunk(attrs.getAttribute("y"));
			if (attrs.getAttribute("width"))
				fDimWidth.loadFromChunk(attrs.getAttribute("width"));
			if (attrs.getAttribute("height"))
				fDimHeight.loadFromChunk(attrs.getAttribute("height"));

			//printf("SVGImageNode: %3.0f %3.0f\n", fWidth, fHeight);
			ByteSpan href = attrs.getAttribute("href");
			if (!href)
				href = attrs.getAttribute("xlink:href");
			
			if (href)
				fImageRef = href;
		}

		void drawSelf(IRenderSVG* ctx) override
		{
			if (fImage.empty())
				return;

			BLRect dst{ fX,fY, fWidth,fHeight };
			BLRectI src{ 0,0,fImage.size().w,fImage.size().h };

			ctx->scaleImage(fImage, src.x, src.y, src.w, src.h, fX, fY, fWidth, fHeight);
		}
		
	};

	

	

	

	
	//============================================================
	//	SVGStyleNode
	// Content node is a CDATA section that contains a style sheet
	// Or it is just the plain content of the node
	// This node exists to load the stylesheet into the root document
	// object.  It should not become a part of the document tree
	// itself.
	//============================================================
	struct SVGStyleNode :public SVGGraphicsElement
	{
		static void registerFactory()
		{
			gSVGGraphicsElementCreation["style"] = [](IAmGroot* aroot, XmlElementIterator& iter) {
				auto node = std::make_shared<SVGStyleNode>(aroot);
				node->loadFromXmlIterator(iter);
				return node;
			};
		}

		
		SVGStyleNode(IAmGroot* aroot) :SVGGraphicsElement(aroot) 
		{
			isStructural(false);
		}
		
		void loadContentNode(const XmlElement& elem) override
		{
			loadCDataNode(elem);
		}
		
		void loadCDataNode(const XmlElement& elem) override
		{
			if (root())
			{
				root()->styleSheet()->loadFromSpan(elem.data());
			}
		}
	};
	


	

	
	//============================================================
	//	SVGGradient
	//  gradientUnits ("userSpaceOnUse" or "objectBoundingBox")
	//============================================================
	enum GradientUnits{
		objectBoundingBox = 0,
		userSpaceOnUse = 1
	};
	
	// Default values
	// offset == 0
	// color == black
	// opacity == 1.0
	struct SVGStopNode : public SVGViewable
	{
		double fOffset = 0;
		double fOpacity = 1;
		BLRgba32 fColor{ 0xff000000 };

		SVGStopNode(IAmGroot* aroot):SVGViewable(aroot) {}
		
		double offset() const { return fOffset; }
		double opacity() const { return fOpacity; }
		BLRgba32 color() const { return fColor; }
		
		void loadFromXmlElement(const XmlElement& elem) override
		{	
			SVGViewable::loadFromXmlElement(elem);
			
			// Get the offset
			SVGDimension dim{};
			dim.loadFromChunk(getAttribute("offset"));
			if (dim.isSet())
			{
				fOffset = dim.calculatePixels(1);
			}
			
			SVGDimension dimOpacity{};
			SVGPaint paint(root());
			
			// We'll get the paint either from a style attribute
			// or from stop-color and stop-opacity attributes
			auto style = getAttribute("style");
			if (style)
			{
				// If we have a style attribute, assume both the stop-color
				// and the stop-opacity are in there

				XmlAttributeCollection styleAttributes;
				parseStyleAttribute(style, styleAttributes);
				paint.loadFromChunk(styleAttributes.getAttribute("stop-color"));

				// load the opacity
				dimOpacity.loadFromChunk(styleAttributes.getAttribute("stop-opacity"));
			}
			else
			{
				// If we don't have a style attribute, assume we have
				// stop-color and possibly stop-opacity attributes
				// otherwise, default to black
				if (getAttribute("stop-color"))
				{
					paint.loadFromChunk(getAttribute("stop-color"));
				}
				else
				{
					// Default color is black
					paint.loadFromChunk("black");
				}

				if (getAttribute("stop-opacity"))
				{
					dimOpacity.loadFromChunk(getAttribute("stop-opacity"));
				}
				else
				{
					// Default opacity is 1.0
					dimOpacity.loadFromChunk("1.0");
				}
			}
			
			if (dimOpacity.isSet())
			{
				fOpacity = dimOpacity.calculatePixels(1);
			}

			paint.setOpacity(fOpacity);
			BLVar aVar = paint.getVariant();
			uint32_t colorValue = 0;
			auto res = blVarToRgba32(&aVar, &colorValue);
			fColor.value = colorValue;

		}
	};
	
	//============================================================
	// SVGColidColor
	// Represents a single solid color
	// You could represent this as a linear gradient with a single color
	// but this is the way to do it for SVG 2.0
	//============================================================
	struct SVGSolidColorElement : public SVGVisualNode
	{
		static void registerFactory() {
			gShapeCreationMap["solidColor"] = [](IAmGroot* root, const XmlElement& elem) {
				auto node = std::make_shared<SVGSolidColorElement>(root);
				node->loadFromXmlElement(elem);
				return node;
			};
		}
		
		SVGPaint fPaint{nullptr};
		
		SVGSolidColorElement(IAmGroot* aroot) :SVGVisualNode(aroot) {}


		const BLVar& getVariant() override
		{
			if (fVar.isNull())
			{
				blVarAssignWeak(&fVar, &fPaint.getVariant());
			}
			
			return fVar;
		}
		
		void loadVisualProperties(const XmlAttributeCollection& attrs) override
		{
			SVGVisualNode::loadVisualProperties(attrs);

			if (attrs.getAttribute("solid-color"))
			{
				fPaint.loadFromChunk(getAttribute("solid-color"));
			}

			if (attrs.getAttribute("solid-opacity"))
			{
				double opa = toDouble(attrs.getAttribute("solid-opacity"));
				fPaint.setOpacity(opa);
			}
		}
		

	
	};
	
	//============================================================
	// SVGGradient
	// Base class for other gradient types
	//============================================================
	struct SVGGradient : public SVGGraphicsElement
	{
		bool fHasGradientTransform = false;
		BLMatrix2D fGradientTransform{};
		
		BLGradient fGradient{};
		BLVar fGradientVar{};
		GradientUnits fGradientUnits{ objectBoundingBox };
		ByteSpan fTemplateReference{};
		
		
		// Constructor
		SVGGradient(IAmGroot* aroot) 
			:SVGGraphicsElement(aroot) 
		{
			fGradient.setExtendMode(BL_EXTEND_MODE_PAD);
			//visible(true);
			isStructural(false);
		}
		SVGGradient(const SVGGradient& other) = delete;
		SVGGradient operator=(const SVGGradient& other) = delete;
		
		const BLVar& getVariant() override
		{	
			if (fGradientVar.isNull())
			{
				fGradientVar = fGradient;
				//blVarAssignWeak(&fGradientVar, &fGradient);
			}
			
			return fGradientVar;
		}
		
		// Load a URL Reference
		void resolveReferences(IAmGroot* groot, const ByteSpan& inChunk)
		{
			ByteSpan str = inChunk;

			auto id = chunk_trim(str, xmlwsp);
			
			// The first character could be '.' or '#'
			// so we need to skip past that
			if (*id == '.' || *id == '#')
				id++;

			if (!id)
				return;

			// lookup the thing we're referencing
			std::string idStr = toString(id);

			if (root() != nullptr)
			{
				auto node = groot->getElementById(idStr);

				
				// pull out the color value
				if (node != nullptr)
				{
					// That node itself might need to be bound
					if (node->needsBinding())
						node->bindToGroot(groot);
					
					const BLVar& aVar = node->getVariant();

					if (aVar.isGradient())
					{
						// BUGBUG - we need to take care here to assign
						// the right stuff based on what kind of gradient
						// we are.
						// Start with just assigning the stops
						const BLGradient & tmpGradient = aVar.as<BLGradient>();
						
						// assign stops from tmpGradient
						fGradient.assignStops(tmpGradient.stops(), tmpGradient.size());
						
						// transform matrix if it already exists and
						// we had set a new one as well
						// otherwise, just set the new one
						if (tmpGradient.hasTransform())
						{
							if (fHasGradientTransform)
							{
								BLMatrix2D tmpMatrix = tmpGradient.transform();
								tmpMatrix.transform(fGradientTransform);
								fGradient.setTransform(tmpMatrix);
							}
							else
							{
								fGradient.setTransform(tmpGradient.transform());
							}
						}
						else if (fHasGradientTransform)
						{
							fGradient.setTransform(fGradientTransform);
						}
						
						/*
						switch (fGradient.type())
						{
						case BL_GRADIENT_TYPE_LINEAR:
							fGradient.setValues(tmpGradient.linear());
							break;
						case BL_GRADIENT_TYPE_RADIAL:
							fGradient.setValues(tmpGradient.radial());
							break;
						case BL_GRADIENT_TYPE_CONICAL:
							fGradient.setValues(tmpGradient.conical());
							break;
						}
						*/
					}
				}
			}
			else {
				printf("SVGGradient::resolveReferences, NO ROOT!!\n");
			}
		}
		
		void bindSelfToGroot(IAmGroot* groot) override
		{	
			if (fTemplateReference) {
				resolveReferences(groot, fTemplateReference);
			}
			else if (fHasGradientTransform)
			{
				fGradient.setTransform(fGradientTransform);
			}
			
			needsBinding(false);
		}
		
		void loadVisualProperties(const XmlAttributeCollection& attrs) override
		{
			SVGGraphicsElement::loadVisualProperties(attrs);

			// See if we have a template reference
			if (attrs.getAttribute("href"))
				fTemplateReference = attrs.getAttribute("href");
			else if (attrs.getAttribute("xlink:href"))
				fTemplateReference = attrs.getAttribute("xlink:href");

			// Whether we've loaded a template or not
			// load the common attributes for gradients
			if (attrs.getAttribute("spreadMethod"))
			{
				auto spread = attrs.getAttribute("spreadMethod");
				if (spread == "pad")
					fGradient.setExtendMode(BL_EXTEND_MODE_PAD);
				else if (spread == "reflect")
					fGradient.setExtendMode(BL_EXTEND_MODE_REFLECT);
				else if (spread == "repeat")
					fGradient.setExtendMode(BL_EXTEND_MODE_REPEAT);
			}

			// read the gradientUnits
			if (attrs.getAttribute("gradientUnits"))
			{
				auto units = attrs.getAttribute("gradientUnits");
				if (units == "userSpaceOnUse")
					fGradientUnits = userSpaceOnUse;
				else if (units == "objectBoundingBox")
					fGradientUnits = objectBoundingBox;
			}

			// Get the transform
			// This will get applied in the resolveReferences in case
			// the template has its own matrix
			if (attrs.getAttribute("gradientTransform"))
			{
				fHasGradientTransform = parseTransform(attrs.getAttribute("gradientTransform"), fGradientTransform);
			}

			needsBinding(true);
		}
		
		//
		// The only nodes here should be stop nodes
		//
		void loadSelfClosingNode(const XmlElement& elem) override
		{
			//printf("SVGGradientNode::loadSelfClosingNode()\n");
			//printXmlElement(elem);
			if (elem.name() != "stop")
			{
				printf("SVGGradientNode::loadSelfClosingNode, unknown node type: %s\n", elem.name().c_str());
				return;
			}
			
			SVGStopNode stop(root());
			stop.loadFromXmlElement(elem);
			
			auto offset = stop.offset();
			auto acolor = stop.color();
			
			fGradient.addStop(offset, acolor);

		}
		
	};
	
	struct SVGLinearGradient : public SVGGradient
	{
		static void registerSingularNode()
		{
			gShapeCreationMap["linearGradient"] = [](IAmGroot* aroot, const XmlElement& elem)  {
				auto node = std::make_shared<SVGLinearGradient>(aroot);
				node->loadFromXmlElement(elem);
				return node;
			};
		}
		
		static void registerFactory()
		{
			gSVGGraphicsElementCreation["linearGradient"] = [](IAmGroot* aroot, XmlElementIterator& iter) {
				auto node = std::make_shared<SVGLinearGradient>(aroot);
				node->loadFromXmlIterator(iter);
				return node;
			};

			registerSingularNode();
		}
		

		SVGDimension fX1{0,SVG_UNITS_USER};
		SVGDimension fY1{ 0,SVG_UNITS_USER };
		SVGDimension fX2{ 100,SVG_UNITS_PERCENT};
		SVGDimension fY2{ 0,SVG_UNITS_PERCENT};
		int fGradientUnits{ userSpaceOnUse };
		
		SVGLinearGradient(IAmGroot* aroot) :SVGGradient(aroot) 
		{
			fGradient.reset();
			fGradient.setType(BL_GRADIENT_TYPE_LINEAR);
		}
		
		void bindSelfToGroot(IAmGroot* groot) override
		{
			SVGGradient::bindSelfToGroot(groot);

			double dpi = 96;
			double w = 1.0;
			double h = 1.0;

			if (nullptr != groot)
			{
				dpi = groot->dpi();
				w = groot->canvasWidth();
				h = groot->canvasHeight();
			}

			
			// Get current values, in case we've already
			// loaded from a template
			BLLinearGradientValues values{0,0,0,1};// = fGradient.linear();

			if (fGradientUnits == userSpaceOnUse)
			{
				if (fX1.isSet())
					values.x0 = fX1.calculatePixels(w, 0, dpi);
				if (fY1.isSet())
					values.y0 = fY1.calculatePixels(h, 0, dpi);
				if (fX2.isSet())
					values.x1 = fX2.calculatePixels(w, 0, dpi);
				if (fY2.isSet())
					values.y1 = fY2.calculatePixels(h, 0, dpi);
			}
			else
			{
				if (fX1.isSet())
					values.x0 = fX1.calculatePixels(w, 0, dpi);
				if (fY1.isSet())
					values.y0 = fY1.calculatePixels(h, 0, dpi);
				if (fX2.isSet())
					values.x1 = fX2.calculatePixels(w, 0, dpi);
				if (fY2.isSet())
					values.y1 = fY2.calculatePixels(h, 0, dpi);
			}
			

			fGradient.setValues(values);
			
			needsBinding(false);
		}
		
		//
		// By implementing this here, we'll get called to load attributes
		// from anywhere, whether presentation properties, inline css or stylesheet
		// This goes beyond what the spec supports
		void loadVisualProperties(const XmlAttributeCollection& attrs) override
		{
			// Make sure to load superclass first to get all the
			// common goodies.
			SVGGradient::loadVisualProperties(attrs);

			if (attrs.getAttribute("x1"))
				fX1.loadFromChunk(attrs.getAttribute("x1"));
			if (attrs.getAttribute("y1"))
				fY1.loadFromChunk(attrs.getAttribute("y1"));
			if (attrs.getAttribute("x2"))
				fX2.loadFromChunk(attrs.getAttribute("x2"));
			if (attrs.getAttribute("y2"))
				fY2.loadFromChunk(attrs.getAttribute("y2"));

			needsBinding(true);
		}
		
	};
	
	//==================================
	// Radial Gradient
				// The radial gradient has a center point (cx, cy), a radius (r), and a focal point (fx, fy)
			// The center point is the center of the circle that the gradient is drawn on
			// The radius is the radius of that outer circle
			// The focal point is the point within, or on the circle that the gradient is focused on
	//==================================
	struct SVGRadialGradient : public SVGGradient
	{
		static void registerSingularNode()
		{
			gShapeCreationMap["radialGradient"] = [](IAmGroot* aroot, const XmlElement& elem) {
				auto node = std::make_shared<SVGRadialGradient>(aroot);
				node->loadFromXmlElement(elem);
				return node;
			};
		}

		static void registerFactory()
		{
			gSVGGraphicsElementCreation["radialGradient"] = [](IAmGroot* aroot, XmlElementIterator& iter) {
				auto node = std::make_shared<SVGRadialGradient>(aroot);
				node->loadFromXmlIterator(iter);
				return node;
			};

			registerSingularNode();
		}

		
		SVGDimension fCx{50, SVG_UNITS_PERCENT};
		SVGDimension fCy{50, SVG_UNITS_PERCENT};
		SVGDimension fR{50, SVG_UNITS_PERCENT};
		SVGDimension fFx{ 50, SVG_UNITS_PERCENT, false };
		SVGDimension fFy{ 50, SVG_UNITS_PERCENT, false };
		
		SVGRadialGradient(IAmGroot* aroot) :SVGGradient(aroot) 
		{
			fGradient.setType(BL_GRADIENT_TYPE_RADIAL);
			
		}

		void bindSelfToGroot(IAmGroot* groot)
		{
			SVGGradient::bindSelfToGroot(groot);

			double dpi = 96;
			double w = 1.0;
			double h = 1.0;

			if (nullptr != groot)
			{
				dpi = groot->dpi();
				w = groot->canvasWidth();
				h = groot->canvasHeight();
			}
			
			
			BLRadialGradientValues values = fGradient.radial();

			values.x0 = fCx.calculatePixels(w, 0, dpi);
			values.y0 = fCy.calculatePixels(h, 0, dpi);
			values.r0 = fR.calculatePixels(w, 0, dpi);

			if (fFx.isSet())
				values.x1 = fFx.calculatePixels(w, 0, dpi);
			else
				values.x1 = values.x0;
			if (fFy.isSet())
				values.y1 = fFy.calculatePixels(h, 0, dpi);
			else
				values.y1 = values.y0;


			fGradient.setValues(values);
			needsBinding(false);
		}
		
		void loadVisualProperties(const XmlAttributeCollection& attrs) override
		{
			SVGGradient::loadVisualProperties(attrs);

			if (attrs.getAttribute("cx"))
				fCx.loadFromChunk(attrs.getAttribute("cx"));
			if (attrs.getAttribute("cy"))
				fCy.loadFromChunk(attrs.getAttribute("cy"));
			if (attrs.getAttribute("r"))
				fR.loadFromChunk(attrs.getAttribute("r"));
			if (attrs.getAttribute("fx"))
				fFx.loadFromChunk(attrs.getAttribute("fx"));
			if (attrs.getAttribute("fy"))
				fFy.loadFromChunk(attrs.getAttribute("fy"));

			needsBinding(true);
		}

	};

	struct SVGConicGradient : public SVGGradient
	{
		static void registerSingularNode()
		{
			gShapeCreationMap["conicGradient"] = [](IAmGroot* aroot, const XmlElement& elem) {
				auto node = std::make_shared<SVGConicGradient>(aroot);
				node->loadFromXmlElement(elem);
				return node;
			};
		}

		static void registerFactory()
		{
			gSVGGraphicsElementCreation["conicGradient"] = [](IAmGroot* aroot, XmlElementIterator& iter) {
				auto node = std::make_shared<SVGConicGradient>(aroot);
				node->loadFromXmlIterator(iter);
				return node;
			};

			registerSingularNode();
		}
		
		SVGDimension x0{};
		SVGDimension y0{};
		SVGDimension angle{};
		SVGDimension repeat{};

		
		SVGConicGradient(IAmGroot* aroot) :SVGGradient(aroot)
		{
			fGradient.setType(BLGradientType::BL_GRADIENT_TYPE_CONIC);
		}


		void bindSelfToGroot(IAmGroot* groot)
		{
			SVGGradient::bindSelfToGroot(groot);

			double dpi = 96;
			double w = 1.0;
			double h = 1.0;

			if (nullptr != groot)
			{
				dpi = groot->dpi();
				w = groot->canvasWidth();
				h = groot->canvasHeight();
			}
			
			BLConicGradientValues values = fGradient.conic();

			if (x0.isSet())
			{
				values.x0 = x0.calculatePixels(w, 0, dpi);
			}

			if (y0.isSet())
			{
				values.y0 = y0.calculatePixels(h, 0, dpi);
			}

			if (angle.isSet())
			{
				// treat the angle as an angle type
				ByteSpan angAttr = getAttribute("angle");
				if (angAttr)
				{
					SVGAngleUnits units;
					parseAngle(angAttr, values.angle, units);
				}
			}
			
			// If there is a specified repeat, then use that
			// otherwise, if the current value is zero, then 
			// set it to one
			if (repeat.isSet())
			{
				values.repeat = repeat.calculatePixels(1.0,0,dpi);
			} else if (values.repeat == 0)
				values.repeat = 1.0;
			

			fGradient.setValues(values);

			
			needsBinding(false);
		}

		
		void loadVisualProperties(const XmlAttributeCollection& attrs) override
		{
			SVGGradient::loadVisualProperties(attrs);
			
			x0.loadFromChunk(attrs.getAttribute("x1"));
			y0.loadFromChunk(attrs.getAttribute("y1"));
			angle.loadFromChunk(attrs.getAttribute("angle"));
			repeat.loadFromChunk(attrs.getAttribute("repeat"));
			
			needsBinding(true);
		}
	};


	//===========================================
	// SVGSymbolNode
	// Notes:  An SVGSymbol can create its own local coordinate system
	// if a viewBox is specified.  We need to take into account the aspectRatio
	// as well to be totally correct.
	// As an enhancement, we'd like to take into account style information
	// such as allowing the width and height to be specified in an style
	// sheet or inline style attribute
	//===========================================
	struct SVGSymbolNode : public SVGGraphicsElement
	{
		static void registerFactory()
		{
			gSVGGraphicsElementCreation["symbol"] = [](IAmGroot* aroot, XmlElementIterator& iter) {
				auto node = std::make_shared<SVGSymbolNode>(aroot); 
				node->loadFromXmlIterator(iter); 
				return node; 
			};
		}
		
		SVGViewbox fViewbox{};
		SVGDimension fRefX{};
		SVGDimension fRefY{};
		SVGDimension fX{};
		SVGDimension fY{};
		SVGDimension fDimWidth{};
		SVGDimension fDimHeight{};

		SVGSymbolNode(IAmGroot* root) : SVGGraphicsElement(root) 
		{
			isStructural(false);
		}

		void applyAttributes(IRenderSVG* ctx) override
		{
			SVGGraphicsElement::applyAttributes(ctx);
			
			auto localSize = ctx->localSize();
			
			// BUGBUG - This is probably supposed to be
			// If the symbol does not specify a width, but
			// the container does, then use that
			// else, if the symbol specifies a width, use that
			if (fViewbox.isSet())
			{
				double scaleX = 1.0;
				double scaleY = 1.0;
				
				// BUGBUG - container size needs to be popped with context push/pop
				// first, if our container has specified a size, then use that
				if (localSize.x > 0 && localSize.y > 0) {
					scaleX = localSize.x / fViewbox.width();
					scaleY = localSize.y / fViewbox.height();
				}
				else if (fDimWidth.isSet() && fDimHeight.isSet())
				{
					scaleX = fDimWidth.calculatePixels() / fViewbox.width();
					scaleY = fDimHeight.calculatePixels() / fViewbox.height();
				}
				
				ctx->scale(scaleX, scaleY);
				ctx->translate(-fViewbox.x(), -fViewbox.y());
				ctx->translate(-fRefX.calculatePixels(), -fRefY.calculatePixels());
			}
		}

		void loadSelfClosingNode(const XmlElement& elem) override
		{
			//printf("SVGSymbolNode::loadSelfClosingNode: \n");
			//printXmlElement(elem);

			auto it = gShapeCreationMap.find(elem.name());
			if (it != gShapeCreationMap.end())
			{
				auto node = it->second(root(), elem);
				addNode(node);
			}

		}

		void loadVisualProperties(const XmlAttributeCollection& attrs) override
		{
			SVGGraphicsElement::loadVisualProperties(attrs);

			if (attrs.getAttribute("viewBox"))
			{
				fViewbox.loadFromChunk(attrs.getAttribute("viewBox"));
			}


			if (attrs.getAttribute("refX"))
			{
				fRefX.loadFromChunk(attrs.getAttribute("refX"));
			}
			if (attrs.getAttribute("refY"))
			{
				fRefY.loadFromChunk(attrs.getAttribute("refY"));
			}

			if (attrs.getAttribute("x"))
			{
				fX.loadFromChunk(attrs.getAttribute("x"));
			}
			if (attrs.getAttribute("y"))
			{
				fY.loadFromChunk(attrs.getAttribute("y"));
			}
			if (attrs.getAttribute("width"))
			{
				fDimWidth.loadFromChunk(attrs.getAttribute("width"));
			}
			if (attrs.getAttribute("height"))
			{
				fDimHeight.loadFromChunk(attrs.getAttribute("height"));
			}
		}
		

	};
	

	//=================================================
	// SVGTitleNode
	// Capture the title of the SVG document
	//=================================================
	struct SVGTitleNode : public SVGGraphicsElement
	{
		static void registerSingularNode()
		{
			gShapeCreationMap["title"] = [](IAmGroot* aroot, const XmlElement& elem) {
				auto node = std::make_shared<SVGTitleNode>(aroot);
				node->loadFromXmlElement(elem);
				node->visible(false);
				
				return node;
			};
		}

		// Static constructor to register factory method in map
		static void registerFactory()
		{
			gSVGGraphicsElementCreation["title"] = [](IAmGroot* aroot, XmlElementIterator& iter) {
				auto node = std::make_shared<SVGTitleNode>(aroot);
				node->loadFromXmlIterator(iter);
				node->visible(false);
				
				return node;
			};

			registerSingularNode();
		}


		// Instance Constructor
		SVGTitleNode(IAmGroot* aroot)
			: SVGGraphicsElement(aroot) {}

		// Load the text content if it exists
		void loadContentNode(const XmlElement& elem) override
		{
			//printf("SVGTitleNode\n");
			//printXmlElement(elem);
			
			// Create a text content node and 
			// add it to our node set
			auto node = std::make_shared<SVGTextContentNode>(root());
			node->text(elem.data());
			addNode(node);
		}
	};
	
	struct SVGDescNode : public SVGGraphicsElement
	{
		static void registerSingularNode()
		{
			gShapeCreationMap["desc"] = [](IAmGroot* aroot, const XmlElement& elem) {
				auto node = std::make_shared<SVGDescNode>(aroot);
				node->loadFromXmlElement(elem);

				return node;
			};
		}

		// Static constructor to register factory method in map
		static void registerFactory()
		{
			gSVGGraphicsElementCreation["desc"] = [](IAmGroot* aroot, XmlElementIterator& iter) {
				auto node = std::make_shared<SVGDescNode>(aroot);
				node->loadFromXmlIterator(iter);

				return node;
			};

			registerSingularNode();
		}


		// Instance Constructor
		SVGDescNode(IAmGroot* aroot)
			: SVGGraphicsElement(aroot) 
		{
			isStructural(false);
			visible(false);
		}

		// Load the text content if it exists
		void loadContentNode(const XmlElement& elem) override
		{
			//printf("SVGTitleNode\n");
			//printXmlElement(elem);

			// Create a text content node and 
			// add it to our node set
			auto node = std::make_shared<SVGTextContentNode>(root());
			node->text(elem.data());
			addNode(node);
		}
	};
	
	//================================================
	// SVGAElement
	// 'a' element
	//================================================
	struct SVGAElement : public SVGGraphicsElement
	{
		static void registerSingularNode()
		{
			gShapeCreationMap["a"] = [](IAmGroot* aroot, const XmlElement& elem) {
				auto node = std::make_shared<SVGAElement>(aroot);
				node->loadFromXmlElement(elem);
				return node;
			};
		}

		// Static constructor to register factory method in map
		static void registerFactory()
		{
			gSVGGraphicsElementCreation["a"] = [](IAmGroot* aroot, XmlElementIterator& iter) {
				auto node = std::make_shared<SVGAElement>(aroot);
				node->loadFromXmlIterator(iter);
				return node;
			};

			registerSingularNode();
		}


		// Instance Constructor
		SVGAElement(IAmGroot* aroot)
			: SVGGraphicsElement(aroot) {}

	};

	//================================================
	// SVGMaskNode
	// 'mask' element
	// Similar to SVGClipPath, except uses SRC_OUT
	//================================================
	struct SVGMaskNode : public SVGGraphicsElement
	{
		static void registerSingularNode()
		{
			gShapeCreationMap["mask"] = [](IAmGroot* aroot, const XmlElement& elem) {
				auto node = std::make_shared<SVGMaskNode>(aroot);
				node->loadFromXmlElement(elem);
				return node;
			};
		}

		// Static constructor to register factory method in map
		static void registerFactory()
		{
			gSVGGraphicsElementCreation["mask"] = [](IAmGroot* aroot, XmlElementIterator& iter) {
				auto node = std::make_shared<SVGMaskNode>(aroot);
				node->loadFromXmlIterator(iter);
				return node;
			};

			registerSingularNode();
		}


		// Instance Constructor
		SVGMaskNode(IAmGroot* aroot)
			: SVGGraphicsElement(aroot) 
		{
			isStructural(false);
			//fUseCacheIsolation = true;
		}


		void drawSelf(IRenderSVG* ctx) override
		{
			//The parent graphic should be stuffed into a BLPattern
			// and set as a fill style, then we can 
			// do this fillMask
			// parent->getVar()
			// BLPattern* pattern = new BLPattern(parent->getVar());
			// ctx->setFillStyle(pattern);
			ctx->fillMask(BLPoint(0, 0), fCachedImage);
			
		}
	};
	
	
	//================================================
	// SVGGroupNode
	// 'g' element
	//================================================
	struct SVGGElement : public SVGGraphicsElement
	{	
		static void registerSingularNode()
		{
			gShapeCreationMap["g"] = [](IAmGroot* aroot, const XmlElement& elem) {
				auto node = std::make_shared<SVGGElement>(aroot);
				node->loadFromXmlElement(elem);
				return node;
			};
		}
		
		// Static constructor to register factory method in map
		static void registerFactory()
		{
			gSVGGraphicsElementCreation["g"] = [](IAmGroot* aroot, XmlElementIterator& iter) {
				auto node = std::make_shared<SVGGElement>(aroot);
				node->loadFromXmlIterator(iter);
				return node;
			};

			registerSingularNode();
		}


		
		// Instance Constructor
		SVGGElement(IAmGroot* aroot)
			: SVGGraphicsElement(aroot)
		{
			//fUseCacheIsolation = true;
		}
	


	};

	//================================================
	// SVGForeignObjectElement
	//================================================
	struct SVGForeignObjectElement : public SVGGraphicsElement
	{
		static void registerFactory()
		{
			gSVGGraphicsElementCreation["foreignObject"] = [](IAmGroot* aroot, XmlElementIterator& iter) {
				auto node = std::make_shared<SVGForeignObjectElement>(aroot);
				node->loadFromXmlIterator(iter);
				return node;
			};

			//registerSingularNode();
		}

		double fX{ 0 };
		double fY{ 0 };
		double fWidth{ 0 };
		double fHeight{ 0 };

		// The intended destination
		SVGDimension fDimX{};
		SVGDimension fDimY{};
		SVGDimension fDimWidth{};
		SVGDimension fDimHeight{};
		
		// Instance Constructor
		SVGForeignObjectElement(IAmGroot* aroot)
			: SVGGraphicsElement(aroot) {}
		
		void bindSelfToGroot(IAmGroot* groot) override
		{

			double dpi = 96;
			double w = 1.0;
			double h = 1.0;

			if (groot)
			{
				dpi = groot->dpi();
				w = groot->canvasWidth();
				h = groot->canvasHeight();
			}

			
			fX = fDimX.calculatePixels(w,0,dpi);
			fY = fDimY.calculatePixels(h,0,dpi);
			fWidth = fDimWidth.calculatePixels(w,0,dpi);
			fHeight = fDimHeight.calculatePixels(h,0,dpi);

		}
		
		
		void loadVisualProperties(const XmlAttributeCollection& attrs) override
		{
			SVGGraphicsElement::loadVisualProperties(attrs);


			if (attrs.getAttribute("x"))
				fDimX.loadFromChunk(attrs.getAttribute("x"));
			if (attrs.getAttribute("y"))
				fDimY.loadFromChunk(attrs.getAttribute("y"));
			if (attrs.getAttribute("width"))
				fDimWidth.loadFromChunk(attrs.getAttribute("width"));
			if (attrs.getAttribute("height"))
				fDimHeight.loadFromChunk(attrs.getAttribute("height"));

		}
	};
	
	//============================================================
	// 	SVGDefsNode
	// This node is here to hold onto definitions of other nodes
	//============================================================
	struct SVGDefsNode : public SVGGraphicsElement
	{
		static void registerSingularNode()
		{
			gShapeCreationMap["defs"] = [](IAmGroot* aroot, const XmlElement& elem) {
				auto node = std::make_shared<SVGDefsNode>(aroot);
				node->loadFromXmlElement(elem);
				//node->visible(false);
				
				return node;
			};
		}
		
		// Static constructor to register factory method in map
		static void registerFactory()
		{
			gSVGGraphicsElementCreation["defs"] = [](IAmGroot* aroot, XmlElementIterator& iter) {
				auto node = std::make_shared<SVGDefsNode>(aroot);
				node->loadFromXmlIterator(iter);
				//node->visible(false);
				
				return node;
			};

			registerSingularNode();
		}

		// Instance Constructor
		SVGDefsNode(IAmGroot* aroot)
			: SVGGraphicsElement(aroot)
		{
			isStructural(false);
			visible(false);
		}

		void drawSelf(IRenderSVG* ctx) override
		{
			printf("==== ERROR ERROR SVGDefsNode::drawSelf ====\n");
		}
	};
	

	//============================================================
	// SVGClipPath
	// Create a SVGSurface that we can draw into
	// get the size by asking for the extent of the enclosed
	// visuals.
	// 
	// At render time, use the clip path in a pattern and fill
	// based on that.
	//============================================================
	struct SVGClipPath : public SVGGraphicsElement
	{
		// Static constructor to register factory method in map
		static void registerFactory()
		{
			gSVGGraphicsElementCreation["clipPath"] = [](IAmGroot* aroot, XmlElementIterator& iter) {
				auto node = std::make_shared<SVGClipPath>(aroot);
				node->loadFromXmlIterator(iter);
				return node;
			};
		}

		BLImage fImage;		// Where we'll render the mask
		
		// Instance Constructor
		SVGClipPath(IAmGroot* aroot)
			: SVGGraphicsElement(aroot)
		{
			isStructural(false);
		}
		
		const BLVar& getVariant() override
		{
			if (!fVar.isNull())
				return fVar;
			
			// get our frame() extent
			BLRect extent = frame();
			
			// create a surface of that size
			// if it's valid
			if (extent.w > 0 && extent.h > 0)
			{
				fImage.create((int)floor((float)extent.w+0.5f), (int)floor((float)extent.h+0.5f), BL_FORMAT_A8);

				// Draw our content into the image
				{
					IRenderSVG ctx(root()->fontHandler());
					ctx.begin(fImage);

					ctx.setCompOp(BL_COMP_OP_SRC_COPY);
					ctx.clearAll();
					ctx.setFillStyle(BLRgba32(0xffffffff));
					ctx.translate(-extent.x, -extent.y);
					draw(&ctx);
					ctx.flush();
				}
				
				// bind our image to fVar for later retrieval
				fVar.assign(fImage);
			}
			
			return fVar;
		}
		

	};
	
	//============================================================
	// 	SVGSwitchElement
	//============================================================
	struct SVGSwitchElement : public SVGGraphicsElement
	{
		static void registerFactory()
		{
			gSVGGraphicsElementCreation["switch"] = [](IAmGroot* aroot, XmlElementIterator& iter) {
				auto node = std::make_shared<SVGSwitchElement>(aroot);
				node->loadFromXmlIterator(iter);
				return node;
			};
		}
		
		
		std::string fSystemLanguage;

		std::unordered_map<std::string, std::shared_ptr<SVGVisualNode>> fLanguageNodes{};
		std::shared_ptr<SVGVisualNode> fDefaultNode{ nullptr };
		std::shared_ptr<SVGVisualNode> fSelectedNode{ nullptr };

		SVGSwitchElement(IAmGroot* root) : SVGGraphicsElement(root) {}


		void bindSelfToGroot(IAmGroot* groot) override
		{

			// Get the system language
			fSystemLanguage = groot->systemLanguage();

			// Find the language specific node
			auto iter = fLanguageNodes.find(fSystemLanguage);
			if (iter != fLanguageNodes.end()) {
				fSelectedNode = iter->second;
			}
			else {
				fSelectedNode = fDefaultNode;
			}
			
			if (nullptr != fSelectedNode)
				fSelectedNode->bindToGroot(groot);
		}
		
		void drawSelf(IRenderSVG* ctx) override
		{
			if (fSelectedNode) {
				fSelectedNode->draw(ctx);
			}
		}

		bool addNode(std::shared_ptr<SVGVisualNode> node) override
		{
			// If the node has a language attribute, add it to the language map
			auto lang = node->getVisualProperty("systemLanguage");
			if (lang) {
				std::string langStr = toString(lang->rawValue());
				fLanguageNodes[langStr] = node;
			}
			else {
				fDefaultNode = node;
			}

			return true;
		}
		

	};
	
	//============================================================
	// SVGPatternNode
	// https://www.svgbackgrounds.com/svg-pattern-guide/#:~:text=1%20Background-size%3A%20cover%3B%20This%20declaration%20is%20good%20when,5%20Background-repeat%3A%20repeat%3B%206%20Background-attachment%3A%20scroll%20%7C%20fixed%3B
	// https://www.svgbackgrounds.com/category/pattern/
	// https://www.visiwig.com/patterns/
	//============================================================
	struct SVGPatternNode :public SVGGraphicsElement
	{
		static void registerSingularNode()
		{
			gShapeCreationMap["pattern"] = [](IAmGroot* aroot, const XmlElement& elem) {
				auto node = std::make_shared<SVGPatternNode>(aroot);
				node->loadFromXmlElement(elem);
				return node;
			};
		}
		
		
		static void registerFactory()
		{
			gSVGGraphicsElementCreation["pattern"] = [](IAmGroot* aroot, XmlElementIterator& iter) {
				auto node = std::make_shared<SVGPatternNode>(aroot);
				node->loadFromXmlIterator(iter);
				return node;
			};

			registerSingularNode();
		}


		BLPattern fPattern{};
		BLMatrix2D fPatternTransform{};
		BLImage fCachedImage{};
		ByteSpan fTemplateReference{};

		// Actual values
		double x{ 0 };
		double y{ 0 };
		double width{ 0 };
		double height{ 0 };
		
		// Raw attribute values
		SVGViewbox fViewbox{};
		SVGDimension fX{};
		SVGDimension fY{};
		SVGDimension fWidth{};
		SVGDimension fHeight{};
		SVGPatternExtendMode fExtendMode{nullptr };

		SVGPatternNode(IAmGroot* aroot) :SVGGraphicsElement(aroot) 
		{
			fPattern.setExtendMode(BL_EXTEND_MODE_PAD);
			fPatternTransform = BLMatrix2D::makeIdentity();
			
			isStructural(false);
		}

		BLRect viewport() const {
			if (fViewbox.isSet()) {
				return fViewbox.fRect;
			}

			// BUGBUG - Right here we need to use IAmGroot to get the size of the window
			return { 0, 0, fWidth.calculatePixels(), fHeight.calculatePixels() };
		}
		
		const BLVar& getVariant() override
		{
			// This should be called
			if (fVar.isNull())
			{
				fVar = fPattern;
				//fVar.assign(fPattern);
			}

			return fVar;
		}
		

		void resolveReferences(IAmGroot* groot, const ByteSpan& inChunk)
		{
			// return early if we can't do a lookup
			if (nullptr == groot)
			{
				printf("SVGPatternNode::resolveReferences - groot == null\n");
				return;
			}
			
			ByteSpan str = inChunk;

			auto id = chunk_trim(str, xmlwsp);

			// The first character could be '.' or '#'
			// so we need to skip past that
			if (*id == '.' || *id == '#')
				id++;

			// return early if we can't do a lookup
			if (!id)
				return;

			// lookup the thing we're referencing
			std::string idStr = toString(id);


			auto node = groot->getElementById(idStr);

			// return early if we could not lookup the node
			if (nullptr == node)
			{
				printf("SVGPatternNode::resolveReferences - node == null\n");
				return;
			}
			

			// That node itself might need to be bound
			if (node->needsBinding())
				node->bindToGroot(groot);

			const BLVar& aVar = node->getVariant();

			if (aVar.isPattern())
			{
				const BLPattern& tmpPattern = aVar.as<BLPattern>();
				
				// pull out whatever values we can inherit
				fPattern = tmpPattern;

				// transform matrix if it already exists and
				// we had set a new one as well
				// otherwise, just set the new one
				/*
				if (fPatternTransform.type() == BL_TRANSFORM_TYPE_IDENTITY)
				{
					if (tmptransform.type() != BL_TRANSFORM_TYPE_IDENTITY)
					{
						fPattern.setTransform(tmptransform);
					}
				}
				else
				{
					if (tmpPattern.transform().type() != BL_TRANSFORM_TYPE_IDENTITY)
					{
						fPattern.applyTransform(tmptransform);
					}
				}
				*/
			}
		}

		// 
		// Resolve template reference
		// fix coordinate system
		//
		void bindSelfToGroot(IAmGroot* groot) override
		{
			if (nullptr == groot)
				return;


			double dpi = 96;
			double cWidth = 1.0;
			double cHeight = 1.0;


			dpi = groot->dpi();
			cWidth = groot->canvasWidth();
			cHeight = groot->canvasHeight();

			//
			// Resolve the templateReference if it exists
			if (fTemplateReference) {
				resolveReferences(groot, fTemplateReference);
			
			
			}
			else {
				//BLRectI bbox{};
				// start out with an initial size for the backing buffer
				int iWidth = (int)cWidth;
				int iHeight = (int)cHeight;

				// If a viewbox exists, then it determines the size
				// of the pattern, and thus the size of the backing store
				if (fViewbox.isSet())
				{
					if (fWidth.isSet() && fHeight.isSet())
					{
						iWidth = (int)fWidth.calculatePixels();
						iHeight = (int)fHeight.calculatePixels();
					}
					else {

						// If a viewbox is set, then the 'width' and 'height' parameters
						// are ignored
						//bbox.reset(0, 0, fViewbox.width(), fViewbox.height());
						iWidth = (int)fViewbox.width();
						iHeight = (int)fViewbox.height();
					}
				}
				else {
					// If we don't have a viewbox, then the bbox can be either
					// 1) A specified width and height
					// 2) A percentage width and height of the pattern's children

					if (fWidth.isSet() && fHeight.isSet())
					{
						BLRect bbox = frame();
						iWidth = (int)bbox.w;
						iHeight = (int)bbox.h;


						// if the units on the fWidth == '%', then take the size of the pattern box
						// and use that as the width, and then calculate the percentage of that
						if (fWidth.units() == SVGDimensionUnits::SVG_UNITS_USER)
						{
							iWidth = (int)fWidth.calculatePixels();
							iHeight = (int)fHeight.calculatePixels();
						}
						else {
							//iWidth = (int)fWidth.calculatePixels();
							//iHeight = (int)fHeight.calculatePixels();

							//iWidth = (int)fWidth.calculatePixels(bbox.w, 0, dpi);
							//iHeight = (int)fHeight.calculatePixels(bbox.h, 0, dpi);
						}

					}
					else {
						BLRect bbox = frame();
						iWidth = (int)bbox.w;
						iHeight = (int)bbox.h;
					}
				}

				// create the backing buffer based on the specified sizes
				fCachedImage.create(iWidth, iHeight, BL_FORMAT_PRGB32);

				// Render out content into the backing buffer
				IRenderSVG ctx(groot->fontHandler());
				ctx.begin(fCachedImage);
				ctx.clearAll();
				ctx.setCompOp(BL_COMP_OP_SRC_COPY);
				ctx.noStroke();
				//ctx.setTransform(fPatternTransform);
				
				draw(&ctx);
				ctx.flush();

				// apply the patternTransform
				// maybe do a translate on x, y?

				// assign that BLImage to the pattern object
				fPattern.setImage(fCachedImage);
			}
			
			
			// Start out with some sizes based on the canvas size
			if (fX.isSet()) {
				x = fX.calculatePixels();
			}
			if (fY.isSet()) {
				y = fY.calculatePixels();
			}
			
			// Whether it was a reference or not, set the extendMode
			if (fExtendMode.isSet())
			{
				fPattern.setExtendMode(fExtendMode.value());
			}
			else {
				fPattern.setExtendMode(BL_EXTEND_MODE_REPEAT);

				}
			
			// Finallly, assign the pattern to the fVar
			// do it in the getVariant() call
		}

		void loadVisualProperties(const XmlAttributeCollection& attrs) override
		{
			SVGGraphicsElement::loadVisualProperties(attrs);

			//parseTransform(getAttribute("patternTransform"), fPatternTransform);

			
			fX.loadFromChunk(getAttribute("x"));
			fY.loadFromChunk(getAttribute("y"));
			fWidth.loadFromChunk(getAttribute("width"));
			fHeight.loadFromChunk(getAttribute("height"));
			fViewbox.loadFromChunk(getAttribute("viewBox"));

			fExtendMode.loadFromChunk(getAttribute("extendMode"));
			
			// See if we have a template reference
			if (attrs.getAttribute("href"))
				fTemplateReference = attrs.getAttribute("href");
			else if (attrs.getAttribute("xlink:href"))
				fTemplateReference = attrs.getAttribute("xlink:href");

		}
	
	};
	

	//====================================================
	// SVGSVGElement
	// This is the root node of an entire SVG tree
	//====================================================
	struct SVGSVGElement : public SVGGraphicsElement
	{
		static void registerFactory()
		{
			gSVGGraphicsElementCreation["svg"] = [](IAmGroot* aroot, XmlElementIterator& iter) {
				auto node = std::make_shared<SVGSVGElement>(aroot);
				node->loadFromXmlIterator(iter);
				return node;
			};

		}

		double fX{ 0 };
		double fY{ 0 };
		double fWidth{ 0 };
		double fHeight{ 0 };
		ViewPort fViewport{};
		
		SVGViewbox fViewbox{};
		SVGDimension fDimWidth{};
		SVGDimension fDimHeight{};
		
		SVGSVGElement(IAmGroot *aroot)
			: SVGGraphicsElement(aroot)
		{
		}

		BLRect frame() const override
		{
			return BLRect(fX, fY, fWidth, fHeight);
		}
		
		/*
		BLRect viewport() const { 
			if (fViewbox.isSet()) {
				return fViewbox.fRect;
			}
			
			// BUGBUG - Right here we need to use IAmGroot to get the size of the window
			if (fDimWidth.isSet() && fDimHeight.isSet()) {
				return BLRect(0, 0, fDimWidth.calculatePixels(), fDimHeight.calculatePixels());
			}
			
			// If no viewbox, width, or height, then we need to use the calculated 
			// bounding box of the document
			return frame();
		}
		*/
		
		void bindSelfToGroot(IAmGroot* groot) override
		{
			// We need to resolve the size of the user space
			// start out with some information from groot
			double dpi = 96;
			double w = 1.0;
			double h = 1.0;

			if (nullptr != groot)
			{
				dpi = groot->dpi();
				w = groot->canvasWidth();
				h = groot->canvasHeight();
			}

			// if we have width and height, then set those
			if (fDimWidth.isSet() && fDimHeight.isSet())
			{
				fWidth = fDimWidth.calculatePixels(w,0,dpi);
				fHeight = fDimHeight.calculatePixels(h,0,dpi);
			}
			else {
				// if the width and height are not explicitly set, then use
				// the canvas size instead
				fWidth = w;
				fHeight = h;
			}
			
			if (fViewbox.isSet())
			{
				fViewport.sceneFrame(fViewbox.fRect);
				fViewport.surfaceFrame(frame());
			}
			else
			{
				fViewport.sceneFrame(frame());
				fViewport.surfaceFrame(frame());

			}

		}
		
		void draw(IRenderSVG *ctx) override
		{
			ctx->push();

			// Start with default state
			//ctx->blendMode(BL_COMP_OP_SRC_OVER);

			//ctx->setStrokeMiterLimit(4.0);
			ctx->strokeJoin(BL_STROKE_JOIN_MITER_CLIP);
			//ctx->strokeJoin(BL_STROKE_JOIN_MITER_BEVEL);
			//ctx->strokeJoin(BL_STROKE_JOIN_MITER_ROUND);
			//ctx->strokeJoin(BL_STROKE_JOIN_ROUND);
			
			ctx->setFillRule(BL_FILL_RULE_NON_ZERO);
			
			ctx->fill(BLRgba32(0, 0, 0));
			ctx->noStroke();
			ctx->strokeWidth(1.0);
			ctx->textAlign(ALIGNMENT::LEFT, ALIGNMENT::BASELINE);
			ctx->textFamily("Arial");
			ctx->textSize(16);
			
			// Apply attributes that have been gathered
			// in the case of the root node, it's mostly the viewport
			applyAttributes(ctx);

			// Apply scaling transform based on viewbox
			//ctx->setTransform(fViewport.sceneToSurfaceTransform());
			
			// Draw the children
			drawChildren(ctx);

			
			ctx->pop();
		}

		void loadVisualProperties(const XmlAttributeCollection& attrs) override
		{
			SVGGraphicsElement::loadVisualProperties(attrs);

			
			fViewbox.loadFromChunk(getAttribute("viewBox"));
			fDimWidth.loadFromChunk(getAttribute("width"));
			fDimHeight.loadFromChunk(getAttribute("height"));
		}
		

	};
	
}



