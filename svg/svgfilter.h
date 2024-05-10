#pragma once

#include <string>
#include <array>
#include <functional>
#include <unordered_map>


#include "svgattributes.h"

//#include "svgpath.h"
#include "css.h"





// Elements related to filters
// filter			- compound
// feGaussianBlur	- single

namespace waavs {


	struct SVGFilterNode : public SVGGraphicsElement
	{
		static void registerSingularNode()
		{
			gShapeCreationMap["filter"] = [](IAmGroot* aroot, const XmlElement& elem) {
				auto node = std::make_shared<SVGFilterNode>(aroot);
				node->loadFromXmlElement(elem);

				return node;
			};
		}
		
		static void registerFactory()
		{
			gSVGGraphicsElementCreation["filter"] = [](IAmGroot* aroot, XmlElementIterator& iter) {
				auto node = std::make_shared<SVGFilterNode>(aroot);
				node->loadFromXmlIterator(iter);
				
				return node;
			};

			registerSingularNode();
		}

		SVGFilterNode(IAmGroot* aroot)
			: SVGGraphicsElement(aroot)
		{
			isStructural(false);
		}

	};

	//=====================================================
	struct SVGFeCompositeNode : public SVGGraphicsElement
	{
		static void registerSingularNode()
		{
			gShapeCreationMap["feComposite"] = [](IAmGroot* aroot, const XmlElement& elem) {
				auto node = std::make_shared<SVGFeCompositeNode>(aroot);
				node->loadFromXmlElement(elem);
				
				return node;
			};
		}

		static void registerFactory()
		{
			gSVGGraphicsElementCreation["feComposite"] = [](IAmGroot* aroot, XmlElementIterator& iter) {
				auto node = std::make_shared<SVGFeCompositeNode>(aroot);
				node->loadFromXmlIterator(iter);
				return node;
			};
			
			registerSingularNode();
		}



		SVGFeCompositeNode(IAmGroot* aroot)
			: SVGGraphicsElement(aroot)
		{
			isStructural(true);
		}

	};

	//=====================================================

	struct SVGFeColorMatrixNode : public SVGGraphicsElement
	{
		static void registerSingularNode()
		{
			gShapeCreationMap["feColorMatrix"] = [](IAmGroot* aroot, const XmlElement& elem) {
				auto node = std::make_shared<SVGFeColorMatrixNode>(aroot);
				node->loadFromXmlElement(elem);
				
				return node;
			};
		}

		static void registerFactory()
		{
			gSVGGraphicsElementCreation["feColorMatrix"] = [](IAmGroot* aroot, XmlElementIterator& iter) {
				auto node = std::make_shared<SVGFeColorMatrixNode>(aroot);
				node->loadFromXmlIterator(iter);
				return node;
			};

			registerSingularNode();
		}


		SVGFeColorMatrixNode(IAmGroot* aroot)
			: SVGGraphicsElement(aroot)
		{
			isStructural(true);
		}

	};

	//=====================================================

	struct SVGFeGaussianBlurNode : public SVGGraphicsElement
	{
		static void registerSingularNode()
		{
			gShapeCreationMap["feGaussianBlur"] = [](IAmGroot* aroot, const XmlElement& elem) {
				auto node = std::make_shared<SVGFeGaussianBlurNode>(aroot);
				node->loadFromXmlElement(elem);
				
				return node;
			};
		}

		static void registerFactory()
		{
			gSVGGraphicsElementCreation["feGaussianBlur"] = [](IAmGroot* aroot, XmlElementIterator& iter) {
				auto node = std::make_shared<SVGFeGaussianBlurNode>(aroot);
				node->loadFromXmlIterator(iter);
				node->visible(false);
				return node;
			};
			
			registerSingularNode();
		}
		


		SVGFeGaussianBlurNode(IAmGroot* aroot)
			: SVGGraphicsElement(aroot)
		{
			isStructural(true);
		}
	};

	//=====================================================

	struct SVGFeTurbulenceNode : public SVGGraphicsElement
	{
		static void registerSingularNode()
		{
			gShapeCreationMap["feTurbulence"] = [](IAmGroot* aroot, const XmlElement& elem) {
				auto node = std::make_shared<SVGFeTurbulenceNode>(aroot);
				node->loadFromXmlElement(elem);

				return node;
			};
		}

		static void registerFactory()
		{
			gSVGGraphicsElementCreation["feTurbulence"] = [](IAmGroot* aroot, XmlElementIterator& iter) {
				auto node = std::make_shared<SVGFeTurbulenceNode>(aroot);
				node->loadFromXmlIterator(iter);
				return node;
			};

			registerSingularNode();
		}



		SVGFeTurbulenceNode(IAmGroot* aroot)
			: SVGGraphicsElement(aroot)
		{
			isStructural(true);
		}
	};
}
