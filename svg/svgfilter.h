#pragma once

#include "svgstructuretypes.h"


#include <string>
#include <array>
#include <functional>
#include <unordered_map>


// Elements related to filters
// filter			- compound


// feBlend			- single
// feColorMatrix	- single
// feComponentTransfer- single
// feComposite		- single
// feConvolveMatrix	- single
// feDiffuseLighting- single
// feDisplacementMap- single
// feDistantLight	- single
// feFlood			- single
// feGaussianBlur	- single
// feImage			- single
// feMerge			- compound
// feMergeNode		- single
// feMorphology		- single
// feOffset			- single
// fePointLight		- single
// feSpecularLighting- single
// feSpotLight		- single
// feTile			- single
// feTurbulence		- single



// feFuncR			- single
// feFuncG			- single
// feFuncB			- single
// feFuncA			- single
//

namespace waavs {

	//
	// filter
	//
	struct SVGFilterElement : public SVGGraphicsElement
	{
		static void registerSingularNode()
		{
			gShapeCreationMap["filter"] = [](IAmGroot* aroot, const XmlElement& elem) {
				auto node = std::make_shared<SVGFilterElement>(aroot);
				node->loadFromXmlElement(elem);

				return node;
			};
		}
		
		static void registerFactory()
		{
			gSVGGraphicsElementCreation["filter"] = [](IAmGroot* aroot, XmlElementIterator& iter) {
				auto node = std::make_shared<SVGFilterElement>(aroot);
				node->loadFromXmlIterator(iter);
				
				return node;
			};

			registerSingularNode();
		}

		// filters need to setup an execution environment.  
		SVGDimension fX{};
		SVGDimension fY{};
		SVGDimension fWidth{};
		SVGDimension fHeight{};
		
		// dictionary of images used in the filter
		std::unordered_map<std::string, BLImage> fFilterImages;
		
		
		SVGFilterElement(IAmGroot* aroot)
			: SVGGraphicsElement(aroot)
		{
			isStructural(false);
		}

		bool addImage(const std::string &name, BLImage &img)
		{
			auto it = fFilterImages.find(name);
			if (it != fFilterImages.end())
				return false;

			fFilterImages[name] = img;
			return true;
		}

		bool getImage(const std::string& name, BLImage &img)
		{
			auto it = fFilterImages.find(name);
			if (it == fFilterImages.end())
				return false;

			img = it->second;
			return true;
		}

		virtual bool addNode(std::shared_ptr < SVGVisualNode > node)
		{
			// if superclass fails to add the node, then forget it
			if (!SVGGraphicsElement::addNode(node))
				return false;
			
			//printf("SVGFeFilter.addNode(%s)\n", node->id().c_str());
			
			return true;
		}

		void loadVisualProperties(const XmlAttributeCollection& attrs) override
		{
			SVGGraphicsElement::loadVisualProperties(attrs);

			fX.loadFromChunk(getAttribute("x"));
			fY.loadFromChunk(getAttribute("y"));
			fWidth.loadFromChunk(getAttribute("width"));
			fHeight.loadFromChunk(getAttribute("height"));

		}
		
	};

	//
	// feBlend
	//
	struct SVGFeBlendElement : public SVGGraphicsElement
	{
		static void registerSingularNode()
		{
			gShapeCreationMap["feBlend"] = [](IAmGroot* aroot, const XmlElement& elem) {
				auto node = std::make_shared<SVGFeBlendElement>(aroot);
				node->loadFromXmlElement(elem);

				return node;
				};
		}

		static void registerFactory()
		{
			gSVGGraphicsElementCreation["feBlend"] = [](IAmGroot* aroot, XmlElementIterator& iter) {
				auto node = std::make_shared<SVGFeBlendElement>(aroot);
				node->loadFromXmlIterator(iter);
				node->visible(false);
				return node;
				};

			registerSingularNode();
		}



		SVGFeBlendElement(IAmGroot* aroot)
			: SVGGraphicsElement(aroot)
		{
			isStructural(true);
		}
	};
	
	//
	// feComponentTransfer
	//
	struct SVGFeComponentTransferElement : public SVGGraphicsElement
	{
		static void registerSingularNode()
		{
			gShapeCreationMap["feComponentTransfer"] = [](IAmGroot* aroot, const XmlElement& elem) {
				auto node = std::make_shared<SVGFeComponentTransferElement>(aroot);
				node->loadFromXmlElement(elem);

				return node;
				};
		}

		static void registerFactory()
		{
			gSVGGraphicsElementCreation["feComponentTransfer"] = [](IAmGroot* aroot, XmlElementIterator& iter) {
				auto node = std::make_shared<SVGFeComponentTransferElement>(aroot);
				node->loadFromXmlIterator(iter);
				return node;
				};

			registerSingularNode();
		}



		SVGFeComponentTransferElement(IAmGroot* aroot)
			: SVGGraphicsElement(aroot)
		{
			isStructural(true);
		}

	};
	
	
	//
	// feComposite
	//
	struct SVGFeCompositeElement : public SVGGraphicsElement
	{
		static void registerSingularNode()
		{
			gShapeCreationMap["feComposite"] = [](IAmGroot* aroot, const XmlElement& elem) {
				auto node = std::make_shared<SVGFeCompositeElement>(aroot);
				node->loadFromXmlElement(elem);
				
				return node;
			};
		}

		static void registerFactory()
		{
			gSVGGraphicsElementCreation["feComposite"] = [](IAmGroot* aroot, XmlElementIterator& iter) {
				auto node = std::make_shared<SVGFeCompositeElement>(aroot);
				node->loadFromXmlIterator(iter);
				return node;
			};
			
			registerSingularNode();
		}



		SVGFeCompositeElement(IAmGroot* aroot)
			: SVGGraphicsElement(aroot)
		{
			isStructural(true);
		}

	};

	//
	// feColorMatrix
	//
	struct SVGFeColorMatrixElement : public SVGGraphicsElement
	{
		static void registerSingularNode()
		{
			gShapeCreationMap["feColorMatrix"] = [](IAmGroot* aroot, const XmlElement& elem) {
				auto node = std::make_shared<SVGFeColorMatrixElement>(aroot);
				node->loadFromXmlElement(elem);
				
				return node;
			};
		}

		static void registerFactory()
		{
			gSVGGraphicsElementCreation["feColorMatrix"] = [](IAmGroot* aroot, XmlElementIterator& iter) {
				auto node = std::make_shared<SVGFeColorMatrixElement>(aroot);
				node->loadFromXmlIterator(iter);
				return node;
			};

			registerSingularNode();
		}


		SVGFeColorMatrixElement(IAmGroot* aroot)
			: SVGGraphicsElement(aroot)
		{
			isStructural(true);
		}

	};

	//
	// feConvolveMatrix
	//
	struct SVGFeConvolveMatrixElement : public SVGGraphicsElement
	{
		static void registerSingularNode()
		{
			gShapeCreationMap["feConvolveMatrix"] = [](IAmGroot* aroot, const XmlElement& elem) {
				auto node = std::make_shared<SVGFeConvolveMatrixElement>(aroot);
				node->loadFromXmlElement(elem);

				return node;
				};
		}

		static void registerFactory()
		{
			gSVGGraphicsElementCreation["feConvolveMatrix"] = [](IAmGroot* aroot, XmlElementIterator& iter) {
				auto node = std::make_shared<SVGFeConvolveMatrixElement>(aroot);
				node->loadFromXmlIterator(iter);
				return node;
				};

			registerSingularNode();
		}


		SVGFeConvolveMatrixElement(IAmGroot* aroot)
			: SVGGraphicsElement(aroot)
		{
			isStructural(true);
		}

	};


	//
	// feDiffuseLighting
	//
	struct SVGFeDiffuseLightingElement : public SVGGraphicsElement
	{
		static void registerSingularNode()
		{
			gShapeCreationMap["feDiffuseLighting"] = [](IAmGroot* aroot, const XmlElement& elem) {
				auto node = std::make_shared<SVGFeDiffuseLightingElement>(aroot);
				node->loadFromXmlElement(elem);

				return node;
				};
		}

		static void registerFactory()
		{
			gSVGGraphicsElementCreation["feDiffuseLighting"] = [](IAmGroot* aroot, XmlElementIterator& iter) {
				auto node = std::make_shared<SVGFeDiffuseLightingElement>(aroot);
				node->loadFromXmlIterator(iter);
				return node;
				};

			registerSingularNode();
		}


		SVGFeDiffuseLightingElement(IAmGroot* aroot)
			: SVGGraphicsElement(aroot)
		{
			isStructural(true);
		}

	};
	
	
	//
	// feDisplacementMap
	//
	struct SVGFeDisplacementMapElement : public SVGGraphicsElement
	{
		static void registerSingularNode()
		{
			gShapeCreationMap["feDisplacementMap"] = [](IAmGroot* aroot, const XmlElement& elem) {
				auto node = std::make_shared<SVGFeDisplacementMapElement>(aroot);
				node->loadFromXmlElement(elem);

				return node;
				};
		}

		static void registerFactory()
		{
			gSVGGraphicsElementCreation["feDisplacementMap"] = [](IAmGroot* aroot, XmlElementIterator& iter) {
				auto node = std::make_shared<SVGFeDisplacementMapElement>(aroot);
				node->loadFromXmlIterator(iter);
				return node;
				};

			registerSingularNode();
		}


		SVGFeDisplacementMapElement(IAmGroot* aroot)
			: SVGGraphicsElement(aroot)
		{
			isStructural(true);
		}

	};

	//
	// feDistantLight
	//
	struct SVGFeDistantLightElement : public SVGGraphicsElement
	{
		static void registerSingularNode()
		{
			gShapeCreationMap["feDistantLight"] = [](IAmGroot* aroot, const XmlElement& elem) {
				auto node = std::make_shared<SVGFeDistantLightElement>(aroot);
				node->loadFromXmlElement(elem);

				return node;
				};
		}

		static void registerFactory()
		{
			gSVGGraphicsElementCreation["feDistantLight"] = [](IAmGroot* aroot, XmlElementIterator& iter) {
				auto node = std::make_shared<SVGFeDistantLightElement>(aroot);
				node->loadFromXmlIterator(iter);
				return node;
				};

			registerSingularNode();
		}


		SVGFeDistantLightElement(IAmGroot* aroot)
			: SVGGraphicsElement(aroot)
		{
			isStructural(true);
		}

	};
	
	//
	// feFlood
	//
	struct SVGFeFloodElement : public SVGGraphicsElement
	{
		static void registerSingularNode()
		{
			gShapeCreationMap["feFlood"] = [](IAmGroot* aroot, const XmlElement& elem) {
				auto node = std::make_shared<SVGFeFloodElement>(aroot);
				node->loadFromXmlElement(elem);

				return node;
				};
		}

		static void registerFactory()
		{
			gSVGGraphicsElementCreation["feFlood"] = [](IAmGroot* aroot, XmlElementIterator& iter) {
				auto node = std::make_shared<SVGFeFloodElement>(aroot);
				node->loadFromXmlIterator(iter);
				node->visible(false);
				return node;
				};

			registerSingularNode();
		}



		SVGFeFloodElement(IAmGroot* aroot)
			: SVGGraphicsElement(aroot)
		{
			isStructural(true);
		}
	};

	
	//
	// feGaussianBlur
	//
	struct SVGFeGaussianBlurElement : public SVGGraphicsElement
	{
		static void registerSingularNode()
		{
			gShapeCreationMap["feGaussianBlur"] = [](IAmGroot* aroot, const XmlElement& elem) {
				auto node = std::make_shared<SVGFeGaussianBlurElement>(aroot);
				node->loadFromXmlElement(elem);
				
				return node;
			};
		}

		static void registerFactory()
		{
			gSVGGraphicsElementCreation["feGaussianBlur"] = [](IAmGroot* aroot, XmlElementIterator& iter) {
				auto node = std::make_shared<SVGFeGaussianBlurElement>(aroot);
				node->loadFromXmlIterator(iter);
				node->visible(false);
				return node;
			};
			
			registerSingularNode();
		}
		

		SVGDimension fStdDeviation;
		
		SVGFeGaussianBlurElement(IAmGroot* aroot)
			: SVGGraphicsElement(aroot)
		{
			isStructural(true);
		}

		void loadVisualProperties(const XmlAttributeCollection& attrs) override
		{
			SVGGraphicsElement::loadVisualProperties(attrs);

			fStdDeviation.loadFromChunk(getAttribute("stdDeviation"));

		}
	};

	//
	// feOffset
	//
	struct SVGFeOffsetElement : public SVGGraphicsElement
	{
		static void registerSingularNode()
		{
			gShapeCreationMap["feOffset"] = [](IAmGroot* aroot, const XmlElement& elem) {
				auto node = std::make_shared<SVGFeOffsetElement>(aroot);
				node->loadFromXmlElement(elem);

				return node;
				};
		}

		static void registerFactory()
		{
			gSVGGraphicsElementCreation["feOffset"] = [](IAmGroot* aroot, XmlElementIterator& iter) {
				auto node = std::make_shared<SVGFeOffsetElement>(aroot);
				node->loadFromXmlIterator(iter);
				node->visible(false);
				return node;
				};

			registerSingularNode();
		}



		SVGFeOffsetElement(IAmGroot* aroot)
			: SVGGraphicsElement(aroot)
		{
			isStructural(true);
		}
	};
	
	
	//
	// feTurbulence
	//
	struct SVGFeTurbulenceElement : public SVGGraphicsElement
	{
		static void registerSingularNode()
		{
			gShapeCreationMap["feTurbulence"] = [](IAmGroot* aroot, const XmlElement& elem) {
				auto node = std::make_shared<SVGFeTurbulenceElement>(aroot);
				node->loadFromXmlElement(elem);

				return node;
			};
		}

		static void registerFactory()
		{
			gSVGGraphicsElementCreation["feTurbulence"] = [](IAmGroot* aroot, XmlElementIterator& iter) {
				auto node = std::make_shared<SVGFeTurbulenceElement>(aroot);
				node->loadFromXmlIterator(iter);
				return node;
			};

			registerSingularNode();
		}



		SVGFeTurbulenceElement(IAmGroot* aroot)
			: SVGGraphicsElement(aroot)
		{
			isStructural(true);
		}
	};
}
