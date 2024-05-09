#include "gifdec.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>

#include <vector>

#include "pixelaccessor.h"
#include "maths.h"

#define MIN(A, B) ((A) < (B) ? (A) : (B))
#define MAX(A, B) ((A) > (B) ? (A) : (B))

using namespace waavs;

typedef struct Entry {
    uint16_t length;
    uint16_t prefix;
    uint8_t  suffix;
} Entry;

typedef struct Table {
    int bulk;
    int nentries;
    Entry* entries;
} Table;


static Table*
new_table(int key_size)
{
    int key;
    int init_bulk = MAX(1 << (key_size + 1), 0x100);
    Table* table = (Table *)malloc(sizeof(*table) + sizeof(Entry) * init_bulk);
    if (table) {
        table->bulk = init_bulk;
        table->nentries = (1 << key_size) + 2;
        table->entries = (Entry*)&table[1];
        for (key = 0; key < (1 << key_size); key++)
            table->entries[key] = { 1, 0xFFF, (uint8_t)key };
    }
    return table;
}

/* Add table entry. Return value:
 *  0 on success
 *  +1 if key size must be incremented after this addition
 *  -1 if could not realloc table */
static int
add_entry(Table** tablep, uint16_t length, uint16_t prefix, uint8_t suffix)
{
    Table* table = *tablep;
    if (table->nentries == table->bulk) {
        table->bulk *= 2;
        table = (Table *)realloc(table, sizeof(*table) + sizeof(Entry) * table->bulk);
        if (!table) return -1;
        table->entries = (Entry*)&table[1];
        *tablep = table;
    }
    table->entries[table->nentries] = { length, prefix, suffix };
    table->nentries++;
    if ((table->nentries & (table->nentries - 1)) == 0)
        return 1;
    return 0;
}



GIFDec::GIFDec(const waavs::ByteSpan &inChunk)
    :fd(inChunk)
{
    uint8_t fdsz;
    uint8_t aspect;
    int i;
    uint8_t *bgcolor;


    
    // Header
    auto sigver = fd.read(3);
    if (sigver != "GIF") {
        fprintf(stderr, "invalid signature\n");
        return ;
    }
    
    // Version
    sigver = fd.read(3);
    if (sigver != "89a") {
        fprintf(stderr, "invalid version\n");
        return ;
    }
    
    // Next 7 bytes, logical Screen Descriptor
    // Width x Height
    fd.read_u16_le(width);
    fd.read_u16_le(height);
    
    // FDSZ
    fd.read_u8(fdsz);
    
    // Presence of GCT
    if (!(fdsz & 0x80)) {
        fprintf(stderr, "no global color table\n");
        return ;
    }
    
    // Color Space's Depth, color resolution
    depth = ((fdsz >> 4) & 7) + 1;

    // Ignore Sort Flag.
    
    // GCT Size
    gct.size = 1 << ((fdsz & 0x07) + 1);
    // Background Color Index
    fd.read_u8(bgindex);

    // Aspect Ratio
    fd.read_u8(aspect);
    
    // END OF LOGICAL SCREEN DESCRIPTOR
    
    // Read Global Color Table
    fd.read_copy(gct.colors, 3 * gct.size);
    palette = &gct;


    
    frame = (uint8_t *)calloc(4, width * height);
    if (!frame) {
        return;
    }
    canvas = &frame[width * height];
    if (bgindex)
        memset(frame, bgindex, width * height);
    bgcolor = &palette->colors[bgindex*3];
    if (bgcolor[0] || bgcolor[1] || bgcolor [2])
        for (i = 0; i < (width * height); i++)
            memcpy(&canvas[i*3], bgcolor, 3);
    
    // Mark where the animation frames start
    anim_start = fd.tell();

}

GIFDec::~GIFDec()
{
    free(frame);
}

void GIFDec::discard_sub_blocks()
{
    uint8_t sz;

    do {
        fd.read_u8(sz);
        fd.skip(sz);
    } while (sz);
}

void GIFDec::read_plain_text_ext()
{
    if (plain_text) {
        uint16_t tx, ty, tw, th;
        uint8_t cw, ch, fg, bg;
        size_t sub_block;
        fd.skip(1);     // block size = 12
        fd.read_u16_le(tx);
        fd.read_u16_le(ty);
        fd.read_u16_le(tw);
        fd.read_u16_le(th);
        fd.read_u8(cw);
        fd.read_u8(ch);
        fd.read_u8(fg);
        fd.read_u8(bg);
        sub_block = fd.tell();
        plain_text(this, tx, ty, tw, th, cw, ch, fg, bg);
        fd.seek(sub_block, BStream::MSEEK_SET);
    } else {
        // Discard plain text metadata.
        fd.skip(13);
    }
    
    // Discard plain text sub-blocks.
    discard_sub_blocks();
}

void GIFDec::read_graphic_control_ext()
{
    uint8_t rdit;

    // Discard block size (always 0x04).
    fd.skip(1);
    
    fd.read_u8(rdit);
    gce.disposal = (rdit >> 2) & 3;
    gce.input = rdit & 2;
    gce.transparency = rdit & 1;
    fd.read_u16_le(gce.delay);
    fd.read_u8(gce.tindex);
    
    // Skip block terminator.
    fd.skip(1);
}

void GIFDec::read_comment_ext()
{
    if (comment) {
        size_t sub_block = fd.tell();
        comment(this);
        fd.seek(sub_block, BStream::MSEEK_SET);
    }
    // Discard comment sub-blocks.
    discard_sub_blocks();
}

void GIFDec::read_application_ext()
{
    waavs::ByteSpan app_id;
    waavs::ByteSpan app_auth_code;

    // Discard block size (always 0x0B).
    fd.seek(1, BStream::MSEEK_CUR);
    // Application Identifier. 
    app_id = fd.read(8);
    // Application Authentication Code.
    app_auth_code = fd.read(3);
    if (app_id == "NETSCAPE") {
        // Discard block size (0x03) and constant byte (0x01).
        fd.seek(2, BStream::MSEEK_CUR);
         fd.read_u16_le(loop_count);
        // Skip block terminator. 
        fd.seek(1, BStream::MSEEK_CUR);
    } else if (application) {
        size_t sub_block = fd.tell();
        application(this, app_id, app_auth_code);
        fd.seek(sub_block, BStream::MSEEK_SET);
        discard_sub_blocks();
    } else {
        discard_sub_blocks();
    }
}

void
GIFDec::read_ext()
{
    uint8_t label;

    fd.read_u8(label);
    switch (label) {
    case 0x01:
        read_plain_text_ext();
        break;
    case 0xF9:
        read_graphic_control_ext();
        break;
    case 0xFE:
        read_comment_ext();
        break;
    case 0xFF:
        read_application_ext();
        break;
    default:
        fprintf(stderr, "unknown extension: %02X\n", label);
    }
}


uint16_t GIFDec::get_key(int key_size, uint8_t &sub_len, uint8_t &shift, uint8_t &abyte)
{
    int bits_read{};
    int rpad{};
    int frag_size{};
    uint16_t key=0;

    for (bits_read = 0; bits_read < key_size; bits_read += frag_size) {
        rpad = (shift + bits_read) % 8;
        if (rpad == 0) {
            // Update byte.
            if (sub_len == 0) {
                fd.read_u8(sub_len); // Must be nonzero!
                if (sub_len == 0)
                    return 0x1000;
            }
            fd.read_u8(abyte);
            sub_len--;
        }
        frag_size = MIN(key_size - bits_read, 8 - rpad);
        key |= ((uint16_t) ((abyte) >> rpad)) << bits_read;
    }
    
    // Clear extra bits to the left.
    key &= (1 << key_size) - 1;
    shift = (shift + key_size) % 8;
    
    return key;
}

// Compute output index of y-th input line, in frame of height h. 
static int
interlaced_line_index(int h, int y)
{
    int p; /* number of lines in current pass */

    p = (h - 1) / 8 + 1;
    if (y < p) // pass 1
        return y * 8;
    y -= p;
    p = (h - 5) / 8 + 1;
    if (y < p) // pass 2 
        return y * 8 + 4;
    y -= p;
    p = (h - 3) / 4 + 1;
    if (y < p) // pass 3 
        return y * 4 + 2;
    y -= p;
    // pass 4
    return y * 2 + 1;
}

// Decompress image pixels.
// Return 0 on success or -1 on out-of-memory (w.r.t. LZW code table).
int GIFDec::read_image_data(int interlace)
{
    uint8_t sub_len{ 0 }, shift{ 0 }, abyte{ 0 };
    int init_key_size{ 0 }, key_size{ 0 }, table_is_full{ 0 };
    int frm_off{ 0 }, frm_size{ 0 }, str_len{ 0 }, i{ 0 }, p{ 0 }, x{ 0 }, y{ 0 };
    uint16_t key{ 0 }, clear{ 0 }, stop{ 0 };
    int ret{ 0 };
    Table* table{ nullptr };
    Entry entry{};
    size_t start{ 0 }, end{ 0 };

    fd.read_u8(abyte);
    key_size = (int) abyte;
    if (key_size < 2 || key_size > 8)
        return -1;
    
    start = fd.tell();
    discard_sub_blocks();
    end = fd.tell();
    fd.seek(start, BStream::MSEEK_SET);
    clear = 1 << key_size;
    stop = clear + 1;
    table = new_table(key_size);
    key_size++;
    init_key_size = key_size;
    sub_len = shift = 0;
    key = get_key(key_size, sub_len, shift, abyte); // clear code
    frm_off = 0;
    ret = 0;
    frm_size = fw*fh;
    while (frm_off < frm_size) {
        if (key == clear) {
            key_size = init_key_size;
            table->nentries = (1 << (key_size - 1)) + 2;
            table_is_full = 0;
        } else if (!table_is_full) {
            ret = add_entry(&table, str_len + 1, key, entry.suffix);
            if (ret == -1) {
                free(table);
                return -1;
            }
            if (table->nentries == 0x1000) {
                ret = 0;
                table_is_full = 1;
            }
        }
        key = get_key(key_size, sub_len, shift, abyte);
        if (key == clear) continue;
        if (key == stop || key == 0x1000) break;
        if (ret == 1) key_size++;
        entry = table->entries[key];
        str_len = entry.length;
        for (i = 0; i < str_len; i++) {
            p = frm_off + entry.length - 1;
            x = p % fw;
            y = p / fw;
            if (interlace)
                y = interlaced_line_index((int) fh, y);
            frame[(fy + y) * width + fx + x] = entry.suffix;
            if (entry.prefix == 0xFFF)
                break;
            else
                entry = table->entries[entry.prefix];
        }
        frm_off += str_len;
        if (key < table->nentries - 1 && !table_is_full)
            table->entries[table->nentries - 1].suffix = entry.suffix;
    }
    free(table);
    if (key == stop)
        fd.read_u8(sub_len); // Must be zero!
    fd.seek(end, BStream::MSEEK_SET);
    
    return 0;
}

// Read image.
// Return true on success or false on out-of-memory (w.r.t. LZW code table).
int
GIFDec::readImage()
{
    uint8_t fisrz;
    int interlace;

    // Image Descriptor.
    fd.read_u16_le(fx);
    fd.read_u16_le(fy);
    
    if (fx >= width || fy >= height)
        return false;
    
    fd.read_u16_le(fw);
    fd.read_u16_le(fh);
    
    fw = MIN(fw, width - fx);
    fh = MIN(fh, height - fy);
    
    fd.read_u8(fisrz);
    interlace = fisrz & 0x40;
    
    // Ignore Sort Flag.
    // Local Color Table?
    if (fisrz & 0x80) {
        // Read LCT
        lct.size = 1 << ((fisrz & 0x07) + 1);
        fd.read_copy(lct.colors, 3 * lct.size);
        palette = &lct;
    } else
        palette = &gct;
    
    // Image Data.
    return read_image_data(interlace);
}

void
GIFDec::render_frame_rect(uint8_t *buffer)
{
    uint8_t index, *color;
    
    int i = (fy * width) + fx;
    for (int j = 0; j < fh; j++) {
        for (int k = 0; k < fw; k++) {
            index = frame[((fy + j) * width) + fx + k];
            color = &palette->colors[index*3];
            if (!gce.transparency || index != gce.tindex)
                memcpy(&buffer[(i+k)*3], color, 3);
        }
        i += width;
    }
}

void GIFDec::disposeFrame()
{
    int i, j, k;
    uint8_t *bgcolor;
    switch (gce.disposal) {
    case 2: // Restore to background color.
        bgcolor = &palette->colors[bgindex*3];
        i = fy * width + fx;
        for (j = 0; j < fh; j++) {
            for (k = 0; k < fw; k++)
                memcpy(&canvas[(i+k)*3], bgcolor, 3);
            i += width;
        }
        break;
    case 3: // Restore to previous, i.e., don't update canvas.
        break;
    default:
        // Add frame non-transparent pixels to canvas.
        render_frame_rect(canvas);
    }
}

// Return 1 if got a frame; 0 if got GIF trailer; -1 if error.
// get the next raw frame of the image
// must call gd_render_frame to decode it
int
GIFDec::getFrame()
{
    uint8_t sep;

    auto sentinel = fd.tell();

    
    disposeFrame();

    sentinel = fd.tell();
    //printf("sentinel: %zd\n", sentinel);
    
    fd.read_u8(sep);

    while (sep != ',') {
        if (sep == ';')
            return 0;
        if (sep == '!')
            read_ext();
        else return -1;
        fd.read_u8(sep);
    }
    
    if (readImage() == -1)
        return -1;
    
    return 1;
}

// gd_render_frame
// decode a frame of the image
void
GIFDec::render_frame(uint8_t *buffer)
{
    memcpy(buffer, canvas, (width * height) * 3);
    render_frame_rect(buffer);
}

//bool
//GIFDec::is_bgcolor(void *acolor)
//{
//    return !memcmp(&palette->colors[bgindex*3], acolor, 3);
//}

//void GIFDec::rewind()
//{
//    fd.seek(anim_start, BStream::MSEEK_SET);
//}





bool parseGIF(void* bytes, const size_t sz, BLImage &outImage)
{
    bool success = false;
    
    waavs::ByteSpan inChunk(bytes, sz);

    GIFDec gif(inChunk);

    // create a chunk of memory to render frames into
    uint8_t* frame = new uint8_t[gif.width * gif.height * 3];

    // At this point we can create the bitmap
    // and fill it with the background color of the image
    outImage.create(gif.width, gif.height, BL_FORMAT_PRGB32);
    BLContext ctx(outImage);
    ctx.clearAll();
    ctx.end();
    
    auto res = gif.getFrame();
    
    if (1 == res)
    {
        // render an individual frame
        gif.render_frame(frame);
        
        // Now copy pixel by pixel
        // Create a pixel accessor for the output
        BLImageData outData;
        outImage.getData(&outData);
        
        PixelAccessor<BLRgba32> imgAccessor(outData.pixelData, outData.size.w, outData.size.h, outData.stride);
        
		// Create a pixel accessor for the input
		PixelAccessor<waavs::vec3b> rgbaccessor(frame, gif.width, gif.height, gif.width * 3);
        
        for (int row=0; row<(int)gif.height; row++)
		{
			for (int col = 0; col < (int)gif.width; col++)
			{
				waavs::vec3b inPixel = rgbaccessor.getPixel(col, row);
                BLRgba32 outPixel( inPixel[0], inPixel[1], inPixel[2] ,255);

                if (!gif.is_bgcolor(&inPixel))
                {
                    // If not the background color
                    // then set the pixel to the specified value
                    imgAccessor.setPixel(col, row, outPixel);
                }
                else {
                    // If it is the background color, explicitly set transparent pixel
                    imgAccessor.setPixel(col, row, { 0,0,0,0 });
                }

			}
		}
        success = true;
    }
    delete[] frame;

    return success;
}