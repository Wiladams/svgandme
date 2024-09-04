#pragma once

#include <string>
#include <array>
#include <functional>
#include <unordered_map>

#include "svgattributes.h"
#include "svgpath.h"
#include "svgtext.h"
#include "viewport.h"
#include "svgmarker.h"





namespace waavs {
	
	//====================================
	// SVGLineElement
	//====================================
	struct SVGLineElement : public SVGGraphicsElement
	{
		static void registerFactory() {
			gShapeCreationMap["line"] = [](IAmGroot* groot, const XmlElement& elem) {
				auto node = std::make_shared<SVGLineElement>(groot);
				node->loadFromXmlElement(elem, groot);
				return node;
			};
		}


		BLLine geom{};

		SVGLineElement(IAmGroot* iMap)
			:SVGGraphicsElement(iMap) {}
		
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
		
		void drawSelf(IRenderSVG* ctx, IAmGroot* groot) override
		{
			ctx->strokeLine(geom);
		}
		
		void resolvePosition(IAmGroot* groot, SVGViewable* container) override
		{	
			double dpi = 96;
			double w = 1.0;
			double h = 1.0;

			if (nullptr != groot)
			{
				dpi = groot->dpi();
			}

			if (nullptr != container)
			{
				BLRect cFrame = container->getBBox();
				w = cFrame.w;
				h = cFrame.h;
			}
			
			SVGDimension fDimX1{};
			SVGDimension fDimY1{};
			SVGDimension fDimX2{};
			SVGDimension fDimY2{};
			
			fDimX1.loadFromChunk(getAttribute("x1"));
			fDimY1.loadFromChunk(getAttribute("y1"));
			fDimX2.loadFromChunk(getAttribute("x2"));
			fDimY2.loadFromChunk(getAttribute("y2"));

			
			if (fDimX1.isSet())
				geom.x0 = fDimX1.calculatePixels(w, 0, dpi);
			if (fDimY1.isSet())
				geom.y0 = fDimY1.calculatePixels(h, 0, dpi);
			if (fDimX2.isSet())
				geom.x1 = fDimX2.calculatePixels(w, 0, dpi);
			if (fDimY2.isSet())
				geom.y1 = fDimY2.calculatePixels(h, 0, dpi);
			
		}
		
	};
	

	//====================================
	// SVGRectElement
	//====================================
	struct SVGRectElement : public SVGGraphicsElement
	{
		static void registerSingular() {
			gShapeCreationMap["rect"] = [](IAmGroot* groot, const XmlElement& elem) {
				auto node = std::make_shared<SVGRectElement>(groot);
				node->loadFromXmlElement(elem, groot);
				return node;
			};
		}
		
		static void registerFactory() {
			gSVGGraphicsElementCreation["rect"] = [](IAmGroot* groot, XmlElementIterator& iter) {
				auto node = std::make_shared<SVGRectElement>(groot);
				node->loadFromXmlIterator(iter, groot);
				
				return node;
				};

			registerSingular();
		}
		
		BLRoundRect geom{};
		bool fIsRound{ false };

		
		SVGRectElement(IAmGroot* iMap) :SVGGraphicsElement(iMap) {}
		
		BLRect getBBox() const override
		{
			return BLRect(geom.x, geom.y, geom.w, geom.h);
		}
		
		void drawSelf(IRenderSVG* ctx, IAmGroot* groot) override
		{
			if (fIsRound) {
				ctx->fillRoundRect(geom);
				ctx->strokeRoundRect(geom);
			}
			else {
				ctx->fillRect(geom.x, geom.y, geom.w, geom.h);
				ctx->strokeRect(geom.x, geom.y, geom.w, geom.h);
			}
		}

		
		void resolvePosition(IAmGroot* groot, SVGViewable* container) override
		{
			double dpi = 96;
			double w = 1.0;
			double h = 1.0;

			if (nullptr != groot)
			{
				dpi = groot->dpi();
			}
			
			if (container)
			{
				BLRect cFrame = container->getBBox();
				w = cFrame.w;
				h = cFrame.h;
			}

			SVGDimension fX{};
			SVGDimension fY{};
			SVGDimension fWidth{};
			SVGDimension fHeight{};
			SVGDimension fRx{};
			SVGDimension fRy{};

			fX.loadFromChunk(getAttribute("x"));
			fY.loadFromChunk(getAttribute("y"));
			fWidth.loadFromChunk(getAttribute("width"));
			fHeight.loadFromChunk(getAttribute("height"));

			fRx.loadFromChunk(getAttribute("rx"));
			fRy.loadFromChunk(getAttribute("ry"));


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

		}
	
	};
	

	//====================================
	//  SVGCircleElement 
	//====================================
	struct SVGCircleElement : public SVGGraphicsElement
	{
		static void registerSingular() {
			gShapeCreationMap["circle"] = [](IAmGroot* groot, const XmlElement& elem) {
				auto node = std::make_shared<SVGCircleElement>(groot);
				node->loadFromXmlElement(elem, groot);
				return node;
				};
		}

		static void registerFactory() {
			gSVGGraphicsElementCreation["circle"] = [](IAmGroot* groot, XmlElementIterator& iter) {
				auto node = std::make_shared<SVGCircleElement>(groot);
				node->loadFromXmlIterator(iter, groot);
				
				return node;
				};

			registerSingular();
		}

		
		BLCircle geom{};


		
		SVGCircleElement(IAmGroot* iMap) :SVGGraphicsElement(iMap) {}

		BLRect getBBox() const override
		{
			return BLRect(geom.cx - geom.r, geom.cy - geom.r, geom.r * 2, geom.r * 2);
		}
		
		void drawSelf(IRenderSVG* ctx, IAmGroot* groot) override
		{
			ctx->fillCircle(geom);
			ctx->strokeCircle(geom);
		}
		
		
		void resolvePosition(IAmGroot *groot, SVGViewable* container) override
		{
			double dpi = 96;
			double w = 1.0;
			double h = 1.0;
			
			if (nullptr != groot)
			{
				dpi = groot->dpi();
			}
			
			if (container) {
				BLRect cFrame = container->getBBox();
				w = cFrame.w;
				h = cFrame.h;
			}
			
			SVGDimension fCx{};
			SVGDimension fCy{};
			SVGDimension fR{};
			
			fCx.loadFromChunk(getAttribute("cx"));
			fCy.loadFromChunk(getAttribute("cy"));
			fR.loadFromChunk(getAttribute("r"));
			
			geom.cx = fCx.calculatePixels(w, 0, dpi);
			geom.cy = fCy.calculatePixels(h, 0, dpi);
			geom.r = fR.calculatePixels(w, h, dpi);

		}
	};
	

	//====================================
	//	SVGEllipseElement 
	//====================================
	struct SVGEllipseElement : public SVGGraphicsElement
	{
		static void registerFactory() {
			registerSVGSingularNode("ellipse", [](IAmGroot* groot, const XmlElement& elem) {
				auto node = std::make_shared<SVGEllipseElement>(groot);
				node->loadFromXmlElement(elem, groot);
				
				return node; });
		}


		BLEllipse geom{};

		
		SVGEllipseElement(IAmGroot* iMap)
			:SVGGraphicsElement(iMap) {}
		
		BLRect getBBox() const override
		{
			return BLRect(geom.cx - geom.rx, geom.cy - geom.ry, geom.rx * 2, geom.ry * 2);
		}

		
		void drawSelf(IRenderSVG* ctx, IAmGroot* groot) override
		{
			ctx->fillEllipse(geom);
			ctx->strokeEllipse(geom);
		}
		
		void resolvePosition(IAmGroot* groot, SVGViewable* container) override
		{
			double dpi = 96;
			double w = 1.0;
			double h = 1.0;
			
			if (nullptr != groot)
			{
				dpi = groot->dpi();
			}
			
			if (nullptr != container)
			{
				BLRect cFrame = container->getBBox();
				w = cFrame.w;
				h = cFrame.h;
			}
			

			SVGDimension fCx{};
			SVGDimension fCy{};
			SVGDimension fRx{};
			SVGDimension fRy{};
			
			fCx.loadFromChunk(getAttribute("cx"));
			fCy.loadFromChunk(getAttribute("cy"));
			fRx.loadFromChunk(getAttribute("rx"));
			fRy.loadFromChunk(getAttribute("ry"));
			
			
			geom.cx = fCx.calculatePixels(w, 0, dpi);
			geom.cy = fCy.calculatePixels(h, 0, dpi);
			geom.rx = fRx.calculatePixels(w, 0, dpi);
			geom.ry = fRy.calculatePixels(h, 0, dpi);

		}

	};
	
	//====================================
	//
	// SVGPathBasedGeometry
	// These are the elements that are based on a path object
	// they support markers, and winding order
	//
	struct SVGPathBasedGeometry : public SVGGraphicsElement
	{
		BLPath fPath{};
		bool fHasMarkers{ false };

		SVGPathBasedGeometry(IAmGroot* iMap)
			:SVGGraphicsElement(iMap)
		{
			//fUseCacheIsolation = true;
		}

		// Return bounding rectangle for shape
		// This does not include the stroke width
		BLRect frame() const override
		{
			// get bounding box, then apply transform
			// BLRect fr = getBBox();

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

			return BLRect(bbox.x0, bbox.y0, (bbox.x1 - bbox.x0), (bbox.y1 - bbox.y0));
		}

		void bindPropertiesToGroot(IAmGroot* groot, SVGViewable* container) override
		{
			SVGGraphicsElement::bindPropertiesToGroot(groot, container);

			//if (((fVisualProperties.find("marker-start") != fVisualProperties.end()) && fVisualProperties["marker-start"]->isSet()) ||
			//	((fVisualProperties.find("marker-mid") != fVisualProperties.end()) && fVisualProperties["marker-mid"]->isSet()) ||
			//	((fVisualProperties.find("marker-end") != fVisualProperties.end()) && fVisualProperties["marker-end"]->isSet()) ||
			//	((fVisualProperties.find("marker") != fVisualProperties.end()) && fVisualProperties["marker"]->isSet()))
			//{
			//	fHasMarkers = true;
			//}

			//needsBinding(false);
		}

		bool drawMarker(IRenderSVG* ctx, std::shared_ptr<SVGVisualProperty> prop, MarkerPosition pos, const BLPoint& p1, const BLPoint& p2, IAmGroot* groot)
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
			std::shared_ptr<SVGMarkerElement> markerNode = std::dynamic_pointer_cast<SVGMarkerElement>(aNode);


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
			markerNode->draw(ctx, groot);


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
		void drawMarkers(IRenderSVG* ctx, IAmGroot* groot)
		{
			// get general marker if it exists
			auto marker = getVisualProperty("marker");
			if (marker != nullptr && marker->needsBinding())
				marker->bindToGroot(groot, this);

			// draw first marker
			auto markerStart = getVisualProperty("marker-start");
			if (markerStart != nullptr && markerStart->isSet() && fPath.size() >= 2)
			{
				// do the binding if necessary
				if (markerStart->needsBinding())
					markerStart->bindToGroot(groot, this);

				// get the first couple of points
				// and draw the single marker
				BLPoint p1 = fPath.vertexData()[0];
				BLPoint p2 = fPath.vertexData()[1];

				drawMarker(ctx, markerStart, MarkerPosition::MARKER_POSITION_START, p1, p2, groot);
			}


			// draw mid-markers
			auto markerMid = getVisualProperty("marker-mid");

			if (nullptr != markerMid && markerMid->isSet() && fPath.size() >= 2)
			{
				// do the binding if necessary
				if (markerMid->needsBinding())
					markerMid->bindToGroot(groot, this);

				for (int i = 1; i < fPath.size() - 1; i++)
				{
					// Get a couple of points
					BLPoint p1 = fPath.vertexData()[i - 1];
					BLPoint p2 = fPath.vertexData()[i];

					drawMarker(ctx, markerMid, MarkerPosition::MARKER_POSITION_MIDDLE, p1, p2, groot);
				}
			}

			// draw last marker
			auto markerEnd = getVisualProperty("marker-end");

			if (nullptr != markerEnd && markerEnd->isSet() && fPath.size() >= 2)
			{
				// do the binding if necessary
				if (markerEnd->needsBinding())
					markerEnd->bindToGroot(groot, this);

				// Get the last two points
				BLPoint p1 = fPath.vertexData()[fPath.size() - 2];
				BLPoint p2 = fPath.vertexData()[fPath.size() - 1];

				drawMarker(ctx, markerEnd, MarkerPosition::MARKER_POSITION_END, p1, p2, groot);
			}

		}

		void drawSelf(IRenderSVG* ctx, IAmGroot* groot) override
		{
			// BUGBUG
			// The paint-order attribute can change which
			// order these are done in
			//printf("SVGGeometryElement::drawSelf(%s)\n", id().c_str());

			ctx->fillPath(fPath);
			ctx->strokePath(fPath);

			// draw markers if we have any
			if (fHasMarkers)
				drawMarkers(ctx, groot);
		}

	};
	
	
	
	
	//====================================
	// SVGPolylineElement
	//
	//====================================
	
	struct SVGPolylineElement : public SVGPathBasedGeometry
	{
		static void registerFactory() {
			gShapeCreationMap["polyline"] = [](IAmGroot* groot, const XmlElement& elem) {
				auto node = std::make_shared<SVGPolylineElement>(groot);
				node->loadFromXmlElement(elem, groot);
				
				return node;
			};
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
		
		void resolvePosition(IAmGroot* groot, SVGViewable* container) override
		{
			loadPoints(getAttribute("points"));
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
			gShapeCreationMap["polygon"] = [](IAmGroot* groot, const XmlElement& elem) {
				auto node = std::make_shared<SVGPolygonElement>(groot);
				node->loadFromXmlElement(elem, groot);
				return node;
				};
		}

		static void registerFactory() {
			gSVGGraphicsElementCreation["polygon"] = [](IAmGroot* groot, XmlElementIterator& iter) {
				auto node = std::make_shared<SVGPolygonElement>(groot);
				node->loadFromXmlIterator(iter, groot);
				
				return node;
				};

			registerSingularNode();
		}

		SVGPolygonElement(IAmGroot* iMap) 
			:SVGPolylineElement(iMap) {}

		void resolvePosition(IAmGroot* groot, SVGViewable* container) override
		{
			SVGPolylineElement::resolvePosition(groot, container);
			fPath.close();
		}

	};
	

	//====================================
	// SVGPath
	// <path>
	//====================================
	
	struct SVGPathElement : public SVGPathBasedGeometry
	{
		static void registerSingularNode() {
			gShapeCreationMap["path"] = [](IAmGroot* groot, const XmlElement& elem) {
				auto node = std::make_shared<SVGPathElement>(groot);
				node->loadFromXmlElement(elem, groot);
				
				return node;
			};
		}

		static void registerFactory()
		{
			gSVGGraphicsElementCreation["path"] = [](IAmGroot* groot, XmlElementIterator& iter) {
				auto node = std::make_shared<SVGPathElement>(groot);
				node->loadFromXmlIterator(iter, groot);
				
				return node;
			};

			registerSingularNode();
		}
		
		SVGPathElement(IAmGroot* iMap) 
			:SVGPathBasedGeometry(iMap)
		{
		}
		
		void loadPath()
		{
			auto d = getAttribute("d");
			if (d) {
				auto success = blpathparser::parsePath(d, fPath);
				fPath.shrink();
			}
		}
		
		void resolvePosition(IAmGroot* groot, SVGViewable* container) override
		{		
			loadPath();
		}


	};

	
	

}



