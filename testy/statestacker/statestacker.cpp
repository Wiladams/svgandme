#include "imanagesvgstate.h"

#pragma comment(lib, "blend2d.lib")


using namespace waavs;

static void writeStyle(BLVar& aStyle)
{
	if (aStyle.isRgba32()) {
		BLRgba32 c{ 0 };
		aStyle.toRgba32(&c);
		printf("0x%x", c.value);
	}
}

static void printState(SVGDrawingState * state)
{
	printf("State: %p\n", state);
	printf("   fCompositeMode: %d\n", state->fCompositeMode);
	printf("        fFillRule: %d\n", state->fFillRule);
	printf("      fPaintOrder: %d\n", state->fPaintOrder);
	printf("   fGlobalOpacity: %f\n", state->fGlobalOpacity);
	printf("   fStrokeOpacity: %f\n", state->fStrokeOpacity);
	printf("     fFillOpacity: %f\n", state->fFillOpacity);
	printf("      fTextCursor: %f, %f\n", state->fTextCursor.x, state->fTextCursor.y);
	printf("  fTextHAlignment: %d\n", state->fTextHAlignment);
	printf("  fTextVAlignment: %d\n", state->fTextVAlignment);
	printf("       fTransform: %f, %f, %f, %f, %f, %f\n",  state->fTransform.m00, state->fTransform.m01, state->fTransform.m10, state->fTransform.m11, state->fTransform.m20, state->fTransform.m21);
	printf("        fClipRect: %f, %f, %f, %f\n", state->fClipRect.x, state->fClipRect.y, state->fClipRect.w, state->fClipRect.h);
	printf("        fViewport: %f, %f, %f, %f\n", state->fViewport.x, state->fViewport.y, state->fViewport.w, state->fViewport.h);
	printf("     fObjectFrame: %f, %f, %f, %f\n", state->fObjectFrame.x, state->fObjectFrame.y, state->fObjectFrame.w, state->fObjectFrame.h);
	printf("     fStrokePaint: "); writeStyle(state->fStrokePaint); printf("\n");
	printf("     fFillPaint: "); writeStyle(state->fFillPaint); printf("\n");
	printf("\n");


}

static void printStack(SVGStateStack& stacker, const char *title=nullptr)
{
	printf("==== State Stack ====\n");
	if (title)
		printf("%s", title);
	printf("\n");

	printf("Entries: %zd\n", stacker.stateStack.size());
	printf("---- Current State ----\n");
	printState(stacker.currentState());
	for (auto& state : stacker.stateStack)
	{
		printState(state);
	}
}

static void testStack1()
{
	SVGStateStack stacker;
	SVGDrawingState* st = stacker.currentState();
	IAccessSVGState acc(st);

	printStack(stacker, "[0]");

	acc.setFillPaint(BLRgba32(0xffff0000));
	stacker.push(); printStack(stacker);

	acc.setFillPaint(BLRgba32(0xff00ff00));
	stacker.push(); printStack(stacker);

	acc.setFillPaint(BLRgba32(0xff0000ff));
	stacker.push(); printStack(stacker);

	
	stacker.pop(); printStack(stacker);
	stacker.pop(); printStack(stacker);
	stacker.pop(); printStack(stacker);
}

int main(int argc, char** argv)
{
	testStack1();

	return 0;
}
