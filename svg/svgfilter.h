#pragma once



#include <string>
#include <array>
#include <functional>
#include <unordered_map>

#include "svgstructuretypes.h"


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
			registerSVGSingularNodeByName("filter", [](IAmGroot* groot, const XmlElement& elem) {
				auto node = std::make_shared<SVGFilterElement>(groot);
				node->loadFromXmlElement(elem, groot);

				return node;
			});
		}
		
		static void registerFactory()
		{
			registerContainerNodeByName("filter", [](IAmGroot* groot, XmlPull& iter) {
				auto node = std::make_shared<SVGFilterElement>(groot);
				node->loadFromXmlPull(iter, groot);
				
				return node;
			});

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
			setIsStructural(false);
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

		//bool addNode(std::shared_ptr < IViewable > node, IAmGroot* groot) override
		//{
			// if superclass fails to add the node, then forget it
		//	if (!SVGGraphicsElement::addNode(node, groot))
		//		return false;
			
			//printf("SVGFeFilter.addNode(%s)\n", node->id().c_str());
			
		//	return true;
		//}

		void bindSelfToContext(IRenderSVG* ctx, IAmGroot* groot) override
		{
			fX.loadFromChunk(getAttributeByName("x"));
			fY.loadFromChunk(getAttributeByName("y"));
			fWidth.loadFromChunk(getAttributeByName("width"));
			fHeight.loadFromChunk(getAttributeByName("height"));

		}
		
	};

	//
	// feBlend
	//
	struct SVGFeBlendElement : public SVGGraphicsElement
	{
		static void registerSingularNode()
		{
			registerSVGSingularNodeByName("feBlend", [](IAmGroot* groot, const XmlElement& elem) {
				auto node = std::make_shared<SVGFeBlendElement>(groot);
				node->loadFromXmlElement(elem, groot);

				return node;
				});
		}

		static void registerFactory()
		{
			registerContainerNodeByName("feBlend", [](IAmGroot* groot, XmlPull& iter) {
				auto node = std::make_shared<SVGFeBlendElement>(groot);
				node->loadFromXmlPull(iter, groot);
				
				return node;
				});

			registerSingularNode();
		}



		SVGFeBlendElement(IAmGroot* )
			: SVGGraphicsElement()
		{
			setIsStructural(true);
			setIsVisible(false);
		}
	};
	
	//
	// feComponentTransfer
	//
	struct SVGFeComponentTransferElement : public SVGGraphicsElement
	{
		static void registerSingularNode()
		{
			registerSVGSingularNodeByName("feComponentTransfer", [](IAmGroot* groot, const XmlElement& elem) {
				auto node = std::make_shared<SVGFeComponentTransferElement>(groot);
				node->loadFromXmlElement(elem, groot);

				return node;
				});
		}

		static void registerFactory()
		{
			registerContainerNodeByName("feComponentTransfer", [](IAmGroot* groot, XmlPull& iter) {
				auto node = std::make_shared<SVGFeComponentTransferElement>(groot);
				node->loadFromXmlPull(iter, groot);
				return node;
				});

			registerSingularNode();
		}



		SVGFeComponentTransferElement(IAmGroot* )
			: SVGGraphicsElement()
		{
			setIsStructural(true);
		}

	};
	
	
	//
	// feComposite
	//
	struct SVGFeCompositeElement : public SVGGraphicsElement
	{
		static void registerSingularNode()
		{
			registerSVGSingularNodeByName("feComposite", [](IAmGroot* groot, const XmlElement& elem) {
				auto node = std::make_shared<SVGFeCompositeElement>(groot);
				node->loadFromXmlElement(elem, groot);
				
				return node;
			});
		}

		static void registerFactory()
		{
			registerContainerNodeByName("feComposite", [](IAmGroot* groot, XmlPull& iter) {
				auto node = std::make_shared<SVGFeCompositeElement>(groot);
				node->loadFromXmlPull(iter, groot);
				
				return node;
			});
			
			registerSingularNode();
		}



		SVGFeCompositeElement(IAmGroot* )
			: SVGGraphicsElement()
		{
			setIsStructural(true);
		}

	};

	//
	// feColorMatrix
	//
	struct SVGFeColorMatrixElement : public SVGGraphicsElement
	{
		static void registerSingularNode()
		{
			registerSVGSingularNodeByName("feColorMatrix", [](IAmGroot* groot, const XmlElement& elem) {
				auto node = std::make_shared<SVGFeColorMatrixElement>(groot);
				node->loadFromXmlElement(elem, groot);
				
				return node;
			});
		}

		static void registerFactory()
		{
			registerContainerNodeByName("feColorMatrix", [](IAmGroot* groot, XmlPull& iter) {
				auto node = std::make_shared<SVGFeColorMatrixElement>(groot);
				node->loadFromXmlPull(iter, groot);
				
				return node;
			});

			registerSingularNode();
		}


		SVGFeColorMatrixElement(IAmGroot* )
			: SVGGraphicsElement()
		{
			setIsStructural(true);
		}

	};

	//
	// feConvolveMatrix
	//
	struct SVGFeConvolveMatrixElement : public SVGGraphicsElement
	{
		static void registerSingularNode()
		{
			registerSVGSingularNodeByName("feConvolveMatrix", [](IAmGroot* groot, const XmlElement& elem) {
				auto node = std::make_shared<SVGFeConvolveMatrixElement>(groot);
				node->loadFromXmlElement(elem, groot);

				return node;
				});
		}

		static void registerFactory()
		{
			registerContainerNodeByName("feConvolveMatrix", [](IAmGroot* groot, XmlPull& iter) {
				auto node = std::make_shared<SVGFeConvolveMatrixElement>(groot);
				node->loadFromXmlPull(iter, groot);
				return node;
				});

			registerSingularNode();
		}


		SVGFeConvolveMatrixElement(IAmGroot* )
			: SVGGraphicsElement()
		{
			setIsStructural(true);
		}

	};


	//
	// feDiffuseLighting
	//
	struct SVGFeDiffuseLightingElement : public SVGGraphicsElement
	{
		static void registerSingularNode()
		{
			registerSVGSingularNodeByName("feDiffuseLighting", [](IAmGroot* groot, const XmlElement& elem) {
				auto node = std::make_shared<SVGFeDiffuseLightingElement>(groot);
				node->loadFromXmlElement(elem, groot);

				return node;
				});
		}

		static void registerFactory()
		{
			registerContainerNodeByName("feDiffuseLighting", [](IAmGroot* groot, XmlPull& iter) {
				auto node = std::make_shared<SVGFeDiffuseLightingElement>(groot);
				node->loadFromXmlPull(iter, groot);
				return node;
				});

			registerSingularNode();
		}


		SVGFeDiffuseLightingElement(IAmGroot* )
			: SVGGraphicsElement()
		{
			setIsStructural(true);
		}

	};
	
	
	//
	// feDisplacementMap
	//
	struct SVGFeDisplacementMapElement : public SVGGraphicsElement
	{
		static void registerSingularNode()
		{
			registerSVGSingularNodeByName("feDisplacementMap", [](IAmGroot* groot, const XmlElement& elem) {
				auto node = std::make_shared<SVGFeDisplacementMapElement>(groot);
				node->loadFromXmlElement(elem, groot);

				return node;
				});
		}

		static void registerFactory()
		{
			registerContainerNodeByName("feDisplacementMap", [](IAmGroot* groot, XmlPull& iter) {
				auto node = std::make_shared<SVGFeDisplacementMapElement>(groot);
				node->loadFromXmlPull(iter, groot);
				return node;
				});

			registerSingularNode();
		}


		SVGFeDisplacementMapElement(IAmGroot* )
			: SVGGraphicsElement()
		{
			setIsStructural(true);
		}

	};

	//
	// feDistantLight
	//
	struct SVGFeDistantLightElement : public SVGGraphicsElement
	{
		static void registerSingularNode()
		{
			registerSVGSingularNodeByName("feDistantLight", [](IAmGroot* groot, const XmlElement& elem) {
				auto node = std::make_shared<SVGFeDistantLightElement>(groot);
				node->loadFromXmlElement(elem, groot);

				return node;
				});
		}

		static void registerFactory()
		{
			registerContainerNodeByName("feDistantLight", [](IAmGroot* groot, XmlPull& iter) {
				auto node = std::make_shared<SVGFeDistantLightElement>(groot);
				node->loadFromXmlPull(iter, groot);
				
				return node;
				});

			registerSingularNode();
		}


		SVGFeDistantLightElement(IAmGroot* )
			: SVGGraphicsElement()
		{
			setIsStructural(true);
		}

	};
	
	//
	// feFlood
	//
	struct SVGFeFloodElement : public SVGGraphicsElement
	{
		static void registerSingularNode()
		{
			registerSVGSingularNodeByName("feFlood", [](IAmGroot* groot, const XmlElement& elem) {
				auto node = std::make_shared<SVGFeFloodElement>(groot);
				node->loadFromXmlElement(elem, groot);

				return node;
				});
		}

		static void registerFactory()
		{
			registerContainerNodeByName("feFlood", [](IAmGroot* groot, XmlPull& iter) {
				auto node = std::make_shared<SVGFeFloodElement>(groot);
				node->loadFromXmlPull(iter, groot);
				
				return node;
				});

			registerSingularNode();
		}



		SVGFeFloodElement(IAmGroot* )
			: SVGGraphicsElement()
		{
			setIsStructural(true);
			setIsVisible(false);
		}
	};

	
	//
	// feGaussianBlur
	//
	struct SVGFeGaussianBlurElement : public SVGGraphicsElement
	{
		static void registerSingularNode()
		{
			registerSVGSingularNodeByName("feGaussianBlur", [](IAmGroot* groot, const XmlElement& elem) {
				auto node = std::make_shared<SVGFeGaussianBlurElement>(groot);
				node->loadFromXmlElement(elem, groot);
				
				return node;
			});
		}

		static void registerFactory()
		{
			registerContainerNodeByName("feGaussianBlur", [](IAmGroot* groot, XmlPull& iter) {
				auto node = std::make_shared<SVGFeGaussianBlurElement>(groot);
				node->loadFromXmlPull(iter, groot);
				
				return node;
			});
			
			registerSingularNode();
		}
		

		SVGDimension fStdDeviation;
		
		SVGFeGaussianBlurElement(IAmGroot* )
			: SVGGraphicsElement()
		{
			setIsStructural(true);
            setIsVisible(false);
		}

		void bindSelfToContext(IRenderSVG* ctx, IAmGroot* groot) override
		{
			fStdDeviation.loadFromChunk(getAttributeByName("stdDeviation"));
		}
	};

	//
	// feOffset
	//
	struct SVGFeOffsetElement : public SVGGraphicsElement
	{
		static void registerSingularNode()
		{
			registerSVGSingularNodeByName("feOffset", [](IAmGroot* groot, const XmlElement& elem) {
				auto node = std::make_shared<SVGFeOffsetElement>(groot);
				node->loadFromXmlElement(elem, groot);

				return node;
				});
		}

		static void registerFactory()
		{
			registerContainerNodeByName("feOffset", [](IAmGroot* groot, XmlPull& iter) {
				auto node = std::make_shared<SVGFeOffsetElement>(groot);
				node->loadFromXmlPull(iter, groot);
				
				return node;
				});

			registerSingularNode();
		}


		SVGFeOffsetElement(IAmGroot* )
			: SVGGraphicsElement()
		{
			setIsStructural(true);
            setIsVisible(false);
		}
	};
	
	
	//
	// feTurbulence
	//
	struct SVGFeTurbulenceElement : public SVGGraphicsElement
	{
		static void registerSingularNode()
		{
			registerSVGSingularNodeByName("feTurbulence", [](IAmGroot* groot, const XmlElement& elem) {
				auto node = std::make_shared<SVGFeTurbulenceElement>(groot);
				node->loadFromXmlElement(elem, groot);

				return node;
			});
		}

		static void registerFactory()
		{
			registerContainerNodeByName("feTurbulence", [](IAmGroot* groot, XmlPull& iter) {
				auto node = std::make_shared<SVGFeTurbulenceElement>(groot);
				node->loadFromXmlPull(iter, groot);
				
				return node;
			});

			registerSingularNode();
		}



		SVGFeTurbulenceElement(IAmGroot* )
			: SVGGraphicsElement()
		{
			setIsStructural(true);
			setIsVisible(false);
		}
	};
}
