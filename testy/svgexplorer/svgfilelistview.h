#pragma once

#include <string>
#include <filesystem>

#include "svgcacheddocument.h"
#include "viewnavigator.h"
#include "svgicons.h"

namespace waavs {


	struct SVGFileIcon : public GraphicView, public Topic<SVGFileIcon>
	{
		static constexpr int sIconSize = 24;

		std::string fFullPath;
		std::string fFilename;
		SVGDocumentHandle fDocument;
		SVGCachedDocument fDocIcon;

		SVGFileIcon(const std::string& name, SVGDocumentHandle doc, const BLRect& rect={0,0,128,24})
			: GraphicView(rect)
			, fFullPath(name)
			, fDocument(doc)
			, fDocIcon(BLRect(0,0, sIconSize, sIconSize), nullptr)
		{
			const std::filesystem::path filePath(name);
			fFilename = filePath.filename().string();

			fDocIcon.resetFromDocument(doc, fDocument->fontHandler());

			ViewNavigator nav;
			nav.setFrame(BLRect(2, 1, sIconSize, sIconSize));
			nav.setBounds(doc->frame());
			fDocIcon.setSceneToSurfaceTransform(nav.sceneToSurfaceTransform());

		}

		const std::string& fileName() const { return fFilename; }
		SVGDocumentHandle document() const { return fDocument; }


		void onMouseEvent(const MouseEvent& e)
		{
			MouseEvent le = e;
			le.x -= static_cast<float>(frame().x);
			le.y -= static_cast<float>(frame().y);
		
			// If it's a click, then do a notify
			if (le.activity == MOUSERELEASED)
			{
				notify(*this);
			}
		}


		void drawForeground(IRenderSVG* ctx) override
		{
			ctx->strokeWidth(3);
			ctx->strokeRect(BLRect(1, 1, frame().w-2, frame().h-2), BLRgba32(0xff7fA0A0));
		}

		void drawSelf(IRenderSVG* ctx)
		{
			fDocIcon.draw(ctx);

			BLRect fr = frame();
			ctx->fillText(fFilename.c_str(), 4+sIconSize, fr.h - 6);
		}

		// Create an instance
		static std::shared_ptr<SVGFileIcon> create(const std::string &name, SVGDocumentHandle doc, const BLRect &fr) 
		{
			auto icon = std::make_shared<SVGFileIcon>(name, doc, fr);
			return icon;
		}
	};
	using SVGFileIconHandle = std::shared_ptr<SVGFileIcon>;



	struct SVGFileListView : public SVGCachedView, public Topic<bool>, public Topic<SVGFileIcon>
	{
		static constexpr size_t sCellHeight = (24 + 2);

		ViewNavigator fNavigator{};
		std::list<SVGFileIconHandle> fFileList{};


		SVGFileListView(const BLRect& aframe, FontHandler *fh)
			:SVGCachedView(aframe, fh)
		{

			fNavigator.setFrame(BLRect(0, 0, aframe.w, aframe.h));
			fNavigator.setBounds(BLRect(0, 0, aframe.w, aframe.h));
			fNavigator.subscribe([this](const bool& e) {this->handleViewChange(e); });

			fCacheContext.background(BLRgba32(0xffffff00));

			setNeedsRedraw(true);
		}


		void handleViewChange(const bool &value)
		{
			//setBounds(fNavigator.bounds());
			setSceneToSurfaceTransform(fNavigator.sceneToSurfaceTransform());

			setNeedsRedraw(true);
			//Topic<bool>::notify(value);
		}

		void handleFileSelected(const SVGFileIcon& fIcon)
		{
			//printf("File Selected: %s\n", fIcon.fileName().c_str());
			Topic<SVGFileIcon>::notify(fIcon);
		}

		bool addFile(const std::string& filename)
		{
			auto mapped = waavs::MappedFile::create_shared(filename);

			// if the mapped file does not exist, return
			if (mapped == nullptr)
			{
				printf("File not found: %s\n", filename.c_str());
				return false;
			}


			ByteSpan aspan(mapped->data(), mapped->size());
			auto doc = SVGDocument::createFromChunk(aspan, &getFontHandler(), canvasWidth, canvasHeight, systemDpi);

			int nFiles = fFileList.size();
			auto anItem = SVGFileIcon::create(filename, doc, BLRect(3,nFiles*(sCellHeight),250,24));
			anItem->subscribe([this](const SVGFileIcon& fIcon) {this->handleFileSelected(fIcon); });

			fFileList.push_back(anItem);

			return true;
		}

		bool getIconIndex(float x, float y, size_t &idx) const
		{
			static constexpr float iconHeight = sCellHeight;
			size_t maxIdx = fFileList.size()-1;

			idx = static_cast<size_t>(y / iconHeight);

			if (idx > maxIdx)
				return false;

			//printf("whichIconIndex: %d  %3.2f, %3.2f\n", index, x, y);

			return true;
		}

		SVGFileIconHandle getIconHandle(size_t idx)
		{
			size_t counter=0;

			for (auto& item : fFileList)
			{
				if (counter == idx)
					return item;

				counter++;
			}
			
			return nullptr;
		}

		void onFileDrop(const FileDropEvent& fde)
		{
			// Clear out any files we currently hold
			fFileList.clear();

			// create a list of file icons to be clicked on
			// assuming there's at least one file that 
			// has been dropped.
			for (int i = 0; i < fde.filenames.size(); i++)
			{
				addFile(fde.filenames[i]);
			}
			
			setNeedsRedraw(true);

			fNavigator.panTo(0, 0);
			// Let whomever is watching know we need to redraw
			//Topic<bool>::notify(true);
		}

		void onMouseEvent(const MouseEvent& e)
		{
			MouseEvent le = e;
			le.x -= static_cast<float>(frame().x);
			le.y -= static_cast<float>(frame().y);
			
			// Adjust mouse event location based on fNavigator transformation
			//fNavigator.
			auto lexy = fNavigator.surfaceToScene(le.x, le.y);
			//printf("LEXY: %3f, %3f\n", lexy.x, lexy.y);
			le.x = lexy.x;
			le.y = lexy.y;

			switch (le.activity)
			{

			case MOUSEWHEEL:
				// Perform vertical scrolling 
				//printf("DELTA: %f\n", le.delta);
				BLRect b = fNavigator.bounds();
				if (le.delta < 0)
				{
					// roll wheel towards user
					// scroll 'down' the list
					double maxY = (fFileList.size()*sCellHeight) - frame().h;
					if (b.y >= maxY)
						return;
				}
				else
				{
					if (b.y <= 0)
						return;
				}

				fNavigator.panBy(0, le.delta*12);
				break;

			case MOUSERELEASED:
			{
				// If a file is selected, then display it
				// Find out which of the icons has been clicked on
				// and notify the observers of this
				size_t idx = 0;
				if (getIconIndex(le.x, le.y, idx))
				{
					auto handle = getIconHandle(idx);
					if (handle != nullptr)
						handle->onMouseEvent(e);
				}

			}
			break;
			}
		}

		void onKeyboardEvent(const KeyboardEvent& ke)
		{
		}

		void drawBackground(IRenderSVG* ctx) override
		{
			ctx->background(BLRgba32(0xffffffff));
		}

		void drawForeground(IRenderSVG* ctx) override
		{
			ctx->strokeWidth(4);
			ctx->strokeRect(BLRect(0, 0, frame().w, frame().h), BLRgba32(0xffA0A0A0));

		}

		void drawSelf(IRenderSVG* ctx)
		{
			ctx->resetFont();

			ctx->fontFamily("Arial");
			ctx->fontSize(16);
			ctx->noStroke();
			ctx->fill(BLRgba32(0xff000000));

			for (auto& item : fFileList) {
				item->draw(ctx);
			}

		}

		void draw(IRenderSVG* ctx) override
		{
			SVGCachedView::draw(ctx);
		}
	};
}
