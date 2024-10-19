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
			getSVGSingularCreationMap()["filter"] = [](IAmGroot* groot, const XmlElement& elem) {
				auto node = std::make_shared<SVGFilterElement>(groot);
				node->loadFromXmlElement(elem, groot);

				return node;
			};
		}
		
		static void registerFactory()
		{
			getSVGContainerCreationMap()["filter"] = [](IAmGroot* groot, XmlElementIterator& iter) {
				auto node = std::make_shared<SVGFilterElement>(groot);
				node->loadFromXmlIterator(iter, groot);
				
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
		
		
		SVGFilterElement(IAmGroot* )
			: SVGGraphicsElement()
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

		virtual bool addNode(std::shared_ptr < ISVGElement > node, IAmGroot* groot)
		{
			// if superclass fails to add the node, then forget it
			if (!SVGGraphicsElement::addNode(node, groot))
				return false;
			
			//printf("SVGFeFilter.addNode(%s)\n", node->id().c_str());
			
			return true;
		}

		void bindSelfToContext(IRenderSVG* ctx, IAmGroot* groot) override
		{
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
			getSVGSingularCreationMap()["feBlend"] = [](IAmGroot* groot, const XmlElement& elem) {
				auto node = std::make_shared<SVGFeBlendElement>(groot);
				node->loadFromXmlElement(elem, groot);

				return node;
				};
		}

		static void registerFactory()
		{
			getSVGContainerCreationMap()["feBlend"] = [](IAmGroot* groot, XmlElementIterator& iter) {
				auto node = std::make_shared<SVGFeBlendElement>(groot);
				node->loadFromXmlIterator(iter, groot);
				node->visible(false);
				
				return node;
				};

			registerSingularNode();
		}



		SVGFeBlendElement(IAmGroot* )
			: SVGGraphicsElement()
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
			getSVGSingularCreationMap()["feComponentTransfer"] = [](IAmGroot* groot, const XmlElement& elem) {
				auto node = std::make_shared<SVGFeComponentTransferElement>(groot);
				node->loadFromXmlElement(elem, groot);

				return node;
				};
		}

		static void registerFactory()
		{
			getSVGContainerCreationMap()["feComponentTransfer"] = [](IAmGroot* groot, XmlElementIterator& iter) {
				auto node = std::make_shared<SVGFeComponentTransferElement>(groot);
				node->loadFromXmlIterator(iter, groot);
				return node;
				};

			registerSingularNode();
		}



		SVGFeComponentTransferElement(IAmGroot* )
			: SVGGraphicsElement()
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
			getSVGSingularCreationMap()["feComposite"] = [](IAmGroot* groot, const XmlElement& elem) {
				auto node = std::make_shared<SVGFeCompositeElement>(groot);
				node->loadFromXmlElement(elem, groot);
				
				return node;
			};
		}

		static void registerFactory()
		{
			getSVGContainerCreationMap()["feComposite"] = [](IAmGroot* groot, XmlElementIterator& iter) {
				auto node = std::make_shared<SVGFeCompositeElement>(groot);
				node->loadFromXmlIterator(iter, groot);
				
				return node;
			};
			
			registerSingularNode();
		}



		SVGFeCompositeElement(IAmGroot* )
			: SVGGraphicsElement()
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
			getSVGSingularCreationMap()["feColorMatrix"] = [](IAmGroot* groot, const XmlElement& elem) {
				auto node = std::make_shared<SVGFeColorMatrixElement>(groot);
				node->loadFromXmlElement(elem, groot);
				
				return node;
			};
		}

		static void registerFactory()
		{
			getSVGContainerCreationMap()["feColorMatrix"] = [](IAmGroot* groot, XmlElementIterator& iter) {
				auto node = std::make_shared<SVGFeColorMatrixElement>(groot);
				node->loadFromXmlIterator(iter, groot);
				
				return node;
			};

			registerSingularNode();
		}


		SVGFeColorMatrixElement(IAmGroot* )
			: SVGGraphicsElement()
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
			getSVGSingularCreationMap()["feConvolveMatrix"] = [](IAmGroot* groot, const XmlElement& elem) {
				auto node = std::make_shared<SVGFeConvolveMatrixElement>(groot);
				node->loadFromXmlElement(elem, groot);

				return node;
				};
		}

		static void registerFactory()
		{
			getSVGContainerCreationMap()["feConvolveMatrix"] = [](IAmGroot* groot, XmlElementIterator& iter) {
				auto node = std::make_shared<SVGFeConvolveMatrixElement>(groot);
				node->loadFromXmlIterator(iter, groot);
				return node;
				};

			registerSingularNode();
		}


		SVGFeConvolveMatrixElement(IAmGroot* )
			: SVGGraphicsElement()
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
			getSVGSingularCreationMap()["feDiffuseLighting"] = [](IAmGroot* groot, const XmlElement& elem) {
				auto node = std::make_shared<SVGFeDiffuseLightingElement>(groot);
				node->loadFromXmlElement(elem, groot);

				return node;
				};
		}

		static void registerFactory()
		{
			getSVGContainerCreationMap()["feDiffuseLighting"] = [](IAmGroot* groot, XmlElementIterator& iter) {
				auto node = std::make_shared<SVGFeDiffuseLightingElement>(groot);
				node->loadFromXmlIterator(iter, groot);
				return node;
				};

			registerSingularNode();
		}


		SVGFeDiffuseLightingElement(IAmGroot* )
			: SVGGraphicsElement()
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
			getSVGSingularCreationMap()["feDisplacementMap"] = [](IAmGroot* groot, const XmlElement& elem) {
				auto node = std::make_shared<SVGFeDisplacementMapElement>(groot);
				node->loadFromXmlElement(elem, groot);

				return node;
				};
		}

		static void registerFactory()
		{
			getSVGContainerCreationMap()["feDisplacementMap"] = [](IAmGroot* groot, XmlElementIterator& iter) {
				auto node = std::make_shared<SVGFeDisplacementMapElement>(groot);
				node->loadFromXmlIterator(iter, groot);
				return node;
				};

			registerSingularNode();
		}


		SVGFeDisplacementMapElement(IAmGroot* )
			: SVGGraphicsElement()
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
			getSVGSingularCreationMap()["feDistantLight"] = [](IAmGroot* groot, const XmlElement& elem) {
				auto node = std::make_shared<SVGFeDistantLightElement>(groot);
				node->loadFromXmlElement(elem, groot);

				return node;
				};
		}

		static void registerFactory()
		{
			getSVGContainerCreationMap()["feDistantLight"] = [](IAmGroot* groot, XmlElementIterator& iter) {
				auto node = std::make_shared<SVGFeDistantLightElement>(groot);
				node->loadFromXmlIterator(iter, groot);
				
				return node;
				};

			registerSingularNode();
		}


		SVGFeDistantLightElement(IAmGroot* )
			: SVGGraphicsElement()
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
			getSVGSingularCreationMap()["feFlood"] = [](IAmGroot* groot, const XmlElement& elem) {
				auto node = std::make_shared<SVGFeFloodElement>(groot);
				node->loadFromXmlElement(elem, groot);

				return node;
				};
		}

		static void registerFactory()
		{
			getSVGContainerCreationMap()["feFlood"] = [](IAmGroot* groot, XmlElementIterator& iter) {
				auto node = std::make_shared<SVGFeFloodElement>(groot);
				node->loadFromXmlIterator(iter, groot);
				node->visible(false);
				
				return node;
				};

			registerSingularNode();
		}



		SVGFeFloodElement(IAmGroot* )
			: SVGGraphicsElement()
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
			getSVGSingularCreationMap()["feGaussianBlur"] = [](IAmGroot* groot, const XmlElement& elem) {
				auto node = std::make_shared<SVGFeGaussianBlurElement>(groot);
				node->loadFromXmlElement(elem, groot);
				
				return node;
			};
		}

		static void registerFactory()
		{
			getSVGContainerCreationMap()["feGaussianBlur"] = [](IAmGroot* groot, XmlElementIterator& iter) {
				auto node = std::make_shared<SVGFeGaussianBlurElement>(groot);
				node->loadFromXmlIterator(iter, groot);
				node->visible(false);
				
				return node;
			};
			
			registerSingularNode();
		}
		

		SVGDimension fStdDeviation;
		
		SVGFeGaussianBlurElement(IAmGroot* )
			: SVGGraphicsElement()
		{
			isStructural(true);
		}

		void bindSelfToContext(IRenderSVG* ctx, IAmGroot* groot) override
		{
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
			getSVGSingularCreationMap()["feOffset"] = [](IAmGroot* groot, const XmlElement& elem) {
				auto node = std::make_shared<SVGFeOffsetElement>(groot);
				node->loadFromXmlElement(elem, groot);

				return node;
				};
		}

		static void registerFactory()
		{
			getSVGContainerCreationMap()["feOffset"] = [](IAmGroot* groot, XmlElementIterator& iter) {
				auto node = std::make_shared<SVGFeOffsetElement>(groot);
				node->loadFromXmlIterator(iter, groot);
				node->visible(false);
				
				return node;
				};

			registerSingularNode();
		}


		SVGFeOffsetElement(IAmGroot* )
			: SVGGraphicsElement()
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
			getSVGSingularCreationMap()["feTurbulence"] = [](IAmGroot* groot, const XmlElement& elem) {
				auto node = std::make_shared<SVGFeTurbulenceElement>(groot);
				node->loadFromXmlElement(elem, groot);

				return node;
			};
		}

		static void registerFactory()
		{
			getSVGContainerCreationMap()["feTurbulence"] = [](IAmGroot* groot, XmlElementIterator& iter) {
				auto node = std::make_shared<SVGFeTurbulenceElement>(groot);
				node->loadFromXmlIterator(iter, groot);
				
				return node;
			};

			registerSingularNode();
		}



		SVGFeTurbulenceElement(IAmGroot* )
			: SVGGraphicsElement()
		{
			isStructural(true);
		}
	};
}
