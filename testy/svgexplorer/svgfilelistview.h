#pragma once

#include <string>
#include <filesystem>

#include "apphost.h"
#include "svgcacheddocument.h"
#include "viewnavigator.h"
#include "svgicons.h"

namespace waavs {


	struct FileIcon : public GraphicView, public Topic<FileIcon>
	{
		std::string fFullPath;
		std::string fFilename;
		SVGDocumentHandle fDocument;
		SVGCachedDocument fDocIcon;
		double fIconSize{};
		bool fIsHover{ false };
		bool fIsSelected{ false };
		
		FileIcon(const std::string& name, SVGDocumentHandle doc, size_t iconSize=24, const BLRect& rect={0,0,128,24})
			: GraphicView(rect)
			, fFullPath(name)
			, fDocument(doc)
			, fDocIcon(BLRect(0,0, iconSize, iconSize))
			, fIconSize(iconSize)
		{
			const std::filesystem::path filePath(name);
			fFilename = filePath.filename().string();

			fDocIcon.resetFromDocument(doc);

            // default alignment is 'xMidYMid meet'
			ViewNavigator nav;

			
			BLRect bbox = doc->topLevelViewPort();
			nav.setFrame(BLRect(2, 1, iconSize, iconSize));
			nav.setBounds(bbox);

			// set the scene to surface transform
			// on the doc icon so it will have the alignment we want
			fDocIcon.setSceneToSurfaceTransform(nav.sceneToSurfaceTransform());
		}

		const std::string& fileName() const { return fFilename; }
		SVGDocumentHandle document() const { return fDocument; }

		void setSelected(bool isSelected) {fIsSelected = isSelected;}
		bool isSelected() const { return fIsSelected; }
		
		void setHover(bool isHover) {fIsHover = isHover;}
		bool isHover() const { return fIsHover; }
		
		// Handle mouse activity
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

		void drawBackground(IRenderSVG* ctx) override 
		{
			BLRect fr = frame();
			ctx->strokeWidth(3);

			BLPath apath;
			BLRect rect(0, 0, fr.w, fr.h);
			apath.addRect(rect);

			if (isSelected())
				ctx->stroke(BLRgba32(0xff7f2f2f));
			else
				ctx->stroke(BLRgba32(0xff7fA0A0));

			ctx->strokeShape(apath);
		}

		void drawForeground(IRenderSVG* ctx) override
		{
			// Draw border around file icon
			BLPath rpath;
			rpath.addRect(0, 0, fDocIcon.frame().w, fDocIcon.frame().h);
			ctx->strokeShape(rpath);
			
			// Draw Draw the icon's filename

			BLRect fr = frame();
			ctx->fill(BLRgba32(0xff000000));
			ctx->fillText(fFilename.c_str(), 4 + fIconSize, fr.h - 6);
			
			// Draw the mouse hover state
			if (isHover())
			{
				BLPath apath;
				apath.addRect(0, 0, fr.w, fr.h);
				ctx->fill(BLRgba32(0x80A0A0A0));
				ctx->fillShape(apath);
			}
		}

		
		void drawSelf(IRenderSVG* ctx)
		{
			fDocIcon.draw(ctx);
		}


	};
	using SVGFileIconHandle = std::shared_ptr<FileIcon>;


	//======================================================
	// 24x24
	struct FileIconSmall : public FileIcon 
	{
		static constexpr int sSmallIconSize = 24;
		
		FileIconSmall(const std::string& name, SVGDocumentHandle doc, const BLRect& fr = { 0,0,128,24 })
			: FileIcon(name, doc, sSmallIconSize, BLRect(fr.x, fr.y, fr.w, sSmallIconSize))
		{
		}

		// Create an instance
		static std::shared_ptr<FileIcon> create(const std::string& name, SVGDocumentHandle doc, const BLRect& fr)
		{
			auto icon = std::make_shared<FileIconSmall>(name, doc, fr);
			return icon;
		}
	};
	

	//======================================================
	// 64x64 icon of file contents
	struct FileIconLarge : public FileIcon 
	{
		static size_t iconSize() { static constexpr size_t sIconSize=64;  return sIconSize; }
		
		FileIconLarge(const std::string& name, SVGDocumentHandle doc, const BLRect& fr = { 0,0,128,24 })
			: FileIcon(name, doc, iconSize(), BLRect(fr.x, fr.y, fr.w, iconSize()))
		{
		}
		
		// Create an instance
		static std::shared_ptr<FileIcon> create(const std::string& name, SVGDocumentHandle doc, const BLRect& fr)
		{
			auto icon = std::make_shared<FileIconLarge>(name, doc, fr);
			return icon;
		}
	};


	

	struct SVGFileListView : public SVGCachedView, public Topic<bool>, public Topic<FileIcon>
	{
		static size_t cellHeight() {
			return FileIconLarge::iconSize() + 2;
		}
	
		
		ViewNavigator fNavigator{};
		std::list<SVGFileIconHandle> fFileList{};
		SVGFileIconHandle fHoverIcon{};
		SVGFileIconHandle fSelectedIcon{};

		SVGFileListView(const BLRect& aframe)
			:SVGCachedView(aframe)
		{

			fNavigator.setFrame(BLRect(0, 0, aframe.w, aframe.h));
			fNavigator.setBounds(BLRect(0, 0, aframe.w, aframe.h));
			fNavigator.subscribe([this](const bool& e) {this->handleViewChange(e); });

			fCacheContext.background(BLRgba32(0xffffff00));

			setNeedsRedraw(true);
		}

		void refresh()
		{
			setNeedsRedraw(true);
			Topic<bool>::notify(true);
		}
		
		void handleViewChange(const bool &value)
		{
			//setBounds(fNavigator.bounds());
			setSceneToSurfaceTransform(fNavigator.sceneToSurfaceTransform());

			refresh();
		}

		void handleFileSelected(const FileIcon& fIcon)
		{
			//printf("File Selected: %s\n", fIcon.fileName().c_str());
			Topic<FileIcon>::notify(fIcon);
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


			ByteSpan aspan;
			aspan.resetFromSize(mapped->data(), mapped->size());
			auto doc = SVGFactory::createFromChunk(aspan, appFrameWidth, appFrameHeight, physicalDpi);

			int nFiles = fFileList.size();
			//auto anItem = FileIconSmall::create(filename, doc, BLRect(3,nFiles*(sCellHeight),250,24));
			auto anItem = FileIconLarge::create(filename, doc, BLRect(3,nFiles*(cellHeight()),250,24));
			anItem->subscribe([this](const FileIcon& fIcon) {this->handleFileSelected(fIcon); });

			fFileList.push_back(anItem);

			return true;
		}

		bool getIconIndex(float x, float y, size_t &idx) const
		{
			static  float iconHeight = cellHeight();
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

		// Handle complex mouse interactions
		void onMouseEvent(const MouseEvent& e)
		{
			// Mouse event is given in window client area coordinates
			// so, we need to convert to bounds coordinates
			MouseEvent le = e;
			le.x -= static_cast<float>(frame().x);
			le.y -= static_cast<float>(frame().y);
			
			// We need to know where the mouse event is in the scene's
			// coordinate system, which is probably transformed.
			auto lexy = fNavigator.surfaceToScene(le.x, le.y);

			le.x = lexy.x;
			le.y = lexy.y;

			// Lastly, we only want to pass wheel scrolls to the navigator
			// as that turns into a pan on the list of files.
			// The mouse released, we deal with locally, as that is a file
			// being selected.
			switch (le.activity)
			{
				case MOUSEMOVED:
				{
					// figure out which icon we're over
					// and draw a highlight over it
					if (fHoverIcon != nullptr)
						fHoverIcon->setHover(false);
					
					size_t idx = 0;
					if (getIconIndex(le.x, le.y, idx))
					{
						fHoverIcon = getIconHandle(idx);
						if (fHoverIcon != nullptr)
						{
							fHoverIcon->setHover(true);
						}

					}
					refresh();
				}
				break;

				case MOUSEWHEEL:
				{
					// Perform vertical scrolling 
					//printf("DELTA: %f\n", le.delta);
					BLRect b = fNavigator.bounds();
					if (le.delta < 0)
					{
						// roll wheel towards user
						// scroll 'down' the list
						double maxY = (fFileList.size() * cellHeight()) - frame().h;
						if (b.y >= maxY)
							return;
					}
					else
					{
						if (b.y <= 0)
							return;
					}

					fNavigator.panBy(0, le.delta * 12);
				}
				break;

				// Can't do mouse pressed yet, because it will allow unconstrained
				// panning.  So, we need to introduce some constraints for the navigator
				//case MOUSEPRESSED:
				//break;
				
				case MOUSERELEASED:
				{
					// If a file is selected, then display it
					// Find out which of the icons has been clicked on
					// and notify the observers of this
					size_t idx = 0;
					if (getIconIndex(le.x, le.y, idx))
					{
						// If there's a currently selected icon, then deselect it
						if (fSelectedIcon != nullptr)
							fSelectedIcon->setSelected(false);

						fSelectedIcon = getIconHandle(idx);
						if (fSelectedIcon != nullptr) {
							fSelectedIcon->setSelected(true);
							fSelectedIcon->onMouseEvent(e);
						}
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
			BLPath apath;
			apath.addRect(0, 0, frame().w, frame().h);
			ctx->stroke(BLRgba32(0xffA0A0A0));
			ctx->strokeShape(apath);

			// draw a semi-transparent rectangle around the hover icon frame
			// get the frame of the hover icon
			//if (fHoverIcon != nullptr)
			//{
			//	BLRect iconFrame = fHoverIcon->frame();
			//	ctx->fillRect(iconFrame, BLRgba32(0x80A0A0A0));
			//}
		}

		void drawSelf(IRenderSVG* ctx)
		{
			ctx->resetFont();

			ctx->setFontFamily("Arial");
			ctx->setFontSize(16);
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
