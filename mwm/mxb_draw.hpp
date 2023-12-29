#include <xcb/xcb.h>
#include <xcb/xcb_image.h>
#include <string>
#include <fstream>
#include <vector>
#include <png.h>
#include <memory>
#include <xcb/xproto.h>

static xcb_screen_t *screen;

class mxb_draw_png 
{
    public:
        mxb_draw_png(const std::string & pngFilePath) : pngFilePath(pngFilePath) {}
        
        void 
        setAsBackground(xcb_connection_t * connection, xcb_window_t win)
        {
            apply_background(connection, win, screen, pngFilePath);
        }

    private:
        std::string pngFilePath;

        struct ImageData 
        {
            std::vector<unsigned char> data;
            int width;
            int height;
        };
  
        ImageData 
        loadPNG(const std::string& filePath) 
        {
            std::ifstream fileStream(filePath, std::ios::binary);
            if (!fileStream) 
            {
                throw std::runtime_error("File " + filePath + " could not be opened for reading");
            }

            png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
            if (!png) 
            {
                throw std::runtime_error("Failed to create PNG read structure");
            }

            png_infop info = png_create_info_struct(png);
            if (!info) 
            {
                png_destroy_read_struct(&png, nullptr, nullptr);
                throw std::runtime_error("Failed to create PNG info structure");
            }

            if (setjmp(png_jmpbuf(png))) 
            {
                png_destroy_read_struct(&png, &info, nullptr);
                throw std::runtime_error("Error during PNG read");
            }

            // Using custom read function to handle C++ streams
            png_set_read_fn(png, static_cast<png_voidp>(&fileStream), [](png_structp pngPtr, png_bytep data, png_size_t length) 
            {
                std::ifstream* stream = static_cast<std::ifstream*>(png_get_io_ptr(pngPtr));
                stream->read(reinterpret_cast<char*>(data), length);
            });

            png_read_info(png, info);

            int width = png_get_image_width(png, info);
            int height = png_get_image_height(png, info);
            png_byte color_type = png_get_color_type(png, info);
            png_byte bit_depth = png_get_bit_depth(png, info);
            
            if(bit_depth == 16) 
            {
                png_set_strip_16(png);
            }

            if(color_type == PNG_COLOR_TYPE_PALETTE) 
            {
                png_set_palette_to_rgb(png);
            }

            if(color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) 
            {
                png_set_expand_gray_1_2_4_to_8(png);
            }

            if(png_get_valid(png, info, PNG_INFO_tRNS)) 
            {
                png_set_tRNS_to_alpha(png);
            }
                        
            if (color_type == PNG_COLOR_TYPE_RGB 
             || color_type == PNG_COLOR_TYPE_GRAY 
             || color_type == PNG_COLOR_TYPE_PALETTE) 
            {
                png_set_filler(png, 0xFF, PNG_FILLER_AFTER);
            }

            if (color_type == PNG_COLOR_TYPE_GRAY 
             || color_type == PNG_COLOR_TYPE_GRAY_ALPHA) 
            {
                png_set_gray_to_rgb(png);
            }

            png_read_update_info(png, info);

            std::vector<unsigned char> imageData;
            imageData.reserve(width * height * 4); // 4 bytes per pixel (RGBA)

            std::unique_ptr<png_bytep[]> row_pointers(new png_bytep[height]);
            for(int y = 0; y < height; y++) {
                row_pointers[y] = new png_byte[png_get_rowbytes(png, info)];
            }

            png_read_image(png, row_pointers.get());

            for(int y = 0; y < height; y++) 
            {
                png_bytep row = row_pointers[y];
                for(int x = 0; x < width; x++) 
                {
                    png_bytep px = &(row[x * 4]);
                    imageData.push_back(px[0]); // Red
                    imageData.push_back(px[1]); // Green
                    imageData.push_back(px[2]); // Blue
                    imageData.push_back(px[3]); // Alpha
                }
                delete[] row_pointers[y];
            }

            png_destroy_read_struct(&png, &info, nullptr);

            return {imageData, width, height};
        }

        ImageData 
        loadPNG_non_cpp_iso(const std::string& filePath) 
        {
            FILE *fp = fopen(filePath.c_str(), "rb");
            if (!fp) 
            {
                throw std::runtime_error("File " + filePath + " could not be opened for reading");
            }

            png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
            if (!png) 
            {
                fclose(fp);
                throw std::runtime_error("Failed to create PNG read structure");
            }

            png_infop info = png_create_info_struct(png);
            if (!info) 
            {
                fclose(fp);
                png_destroy_read_struct(&png, nullptr, nullptr);
                throw std::runtime_error("Failed to create PNG info structure");
            }

            if (setjmp(png_jmpbuf(png))) 
            {
                fclose(fp);
                png_destroy_read_struct(&png, &info, nullptr);
                throw std::runtime_error("Error during PNG read");
            }

            png_init_io(png, fp);
            png_read_info(png, info);

            int width = png_get_image_width(png, info);
            int height = png_get_image_height(png, info);
            png_byte color_type = png_get_color_type(png, info);
            png_byte bit_depth = png_get_bit_depth(png, info);

            if(bit_depth == 16) 
            {
                png_set_strip_16(png);
            }

            if(color_type == PNG_COLOR_TYPE_PALETTE) 
            {
                png_set_palette_to_rgb(png);
            }

            if(color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) 
            {
                png_set_expand_gray_1_2_4_to_8(png);
            }

            if(png_get_valid(png, info, PNG_INFO_tRNS)) 
            {
                png_set_tRNS_to_alpha(png);
            }
                        
            if (color_type == PNG_COLOR_TYPE_RGB 
            || color_type == PNG_COLOR_TYPE_GRAY 
            || color_type == PNG_COLOR_TYPE_PALETTE) 
            {
                png_set_filler(png, 0xFF, PNG_FILLER_AFTER);
            }

            if (color_type == PNG_COLOR_TYPE_GRAY 
            || color_type == PNG_COLOR_TYPE_GRAY_ALPHA) 
            {
                png_set_gray_to_rgb(png);
            }

            png_read_update_info(png, info);

            std::vector<unsigned char> imageData;
            imageData.reserve(width * height * 4); // 4 bytes per pixel (RGBA)

            png_bytep row_pointers[height];
            for(int y = 0; y < height; y++) 
            {
                row_pointers[y] = (png_byte*)malloc(png_get_rowbytes(png, info));
            }

            png_read_image(png, row_pointers);

            fclose(fp);

            for(int y = 0; y < height; y++) 
            {
                png_bytep row = row_pointers[y];
                for(int x = 0; x < width; x++) 
                {
                    png_bytep px = &(row[x * 4]);
                    imageData.push_back(px[0]); // Red
                    imageData.push_back(px[1]); // Green
                    imageData.push_back(px[2]); // Blue
                    imageData.push_back(px[3]); // Alpha
                }
                free(row_pointers[y]);
            }

            png_destroy_read_struct(&png, &info, nullptr);

            return {imageData, width, height};
        }

        xcb_image_t* 
        create_xcb_image(const ImageData& imgData, xcb_connection_t *connection, xcb_screen_t *screen) 
        {
            xcb_image_t *image = xcb_image_create
            (
                imgData.width, imgData.height, 
                XCB_IMAGE_FORMAT_Z_PIXMAP, 
                screen->root_depth, 
                0, 
                32, 
                0, 
                XCB_IMAGE_ORDER_LSB_FIRST, 
                XCB_IMAGE_ORDER_LSB_FIRST, 
                nullptr, 
                ~0, 
                nullptr
            );

            if (!image) 
            {
                return nullptr;
            }

            for (int y = 0; y < imgData.height; ++y) 
            {
                for (int x = 0; x < imgData.width; ++x) 
                {
                    int idx = (y * imgData.width + x) * 4;
                    uint32_t pixel = (imgData.data[idx + 3] << 24) |
                                     (imgData.data[idx + 2] << 16) | 
                                     (imgData.data[idx + 1] << 8)  | 
                                     (imgData.data[idx]);
                    xcb_image_put_pixel(image, x, y, pixel);
                }
            }

            return image;
        }

        void 
        apply_background(xcb_connection_t *connection, xcb_window_t window, xcb_screen_t *screen, const std::string& image_path) 
        {
            ImageData imgData = loadPNG_non_cpp_iso(image_path);
            xcb_image_t *image = create_xcb_image(imgData, connection, screen);
            if (!image) 
            {
                return;
            }

            xcb_pixmap_t pixmap = xcb_generate_id(connection);
            xcb_create_pixmap(connection, screen->root_depth, pixmap, window, imgData.width, imgData.height);
            xcb_gcontext_t gc = xcb_generate_id(connection);
            uint32_t values[2] = {0, 0};
            xcb_create_gc(connection, gc, pixmap, XCB_GC_FOREGROUND | XCB_GC_BACKGROUND, values);

            xcb_image_put(connection, pixmap, gc, image, 0, 0, 0);
            xcb_change_window_attributes(connection, window, XCB_CW_BACK_PIXMAP, &pixmap);
            xcb_clear_area(connection, 0, window, 0, 0, imgData.width, imgData.height);
            xcb_flush(connection);

            xcb_free_pixmap(connection, pixmap);
            xcb_free_gc(connection, gc);
            xcb_image_destroy(image);
        }
};