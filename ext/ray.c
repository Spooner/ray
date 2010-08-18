#include "ray.h"

VALUE ray_mRay = Qnil;

/* Inits ray */
VALUE ray_init(VALUE self) {
   SDL_Init(SDL_INIT_VIDEO);
   return Qnil;
}

/* Stops ray */
VALUE ray_stop(VALUE self) {
   SDL_Quit();
   return Qnil;
}

/*
  Creates a new window.

  @note If both hw_surface and sws_urface are false, hw_surface
        will be considered as true. :sw_surface should be true
        if you want to acess

  @return [Ray::Image] An image representing the window

  @option hash [Integer] :width Width of the window
  @option hash [Integer] :height Height of the window

  @option hash [Integer] :w Alias for width
  @option hash [Integer] :h Alias for height

  @option hash [Integer] :bits_per_pixel Bits per pixel. Valid values are
                                         8, 15, 16, 24, and 32.
  @option hash [Integer] :bpp Alias for bits_per_pixel

  @option hash [true, false] :hw_surface Creates the surface in video memory
                                        (default)
  @option hash [true, false] :sw_surface Creates the surface in system memory
  @option hash [true, false] :async_blit Enables asynchronous updates of the
                                         the surface
  @option hash [true, false] :double_buf Enables double buffering. Ignored
                                         if sw_surface is set.
  @option hash [true, false] :fullscreen Creates a full screen window.
  @option hash [true, false] :resizable  Creates a resizable window.
  @option hash [true, false] :no_frame   Disables window decoration if
                                         possible.
 */
VALUE ray_create_window(VALUE self, VALUE hash) {
   VALUE width  = rb_hash_aref(hash, RAY_SYM("width"));
   VALUE height = rb_hash_aref(hash, RAY_SYM("height"));

   if (NIL_P(width))  width  = rb_hash_aref(hash, RAY_SYM("h"));
   if (NIL_P(height)) height = rb_hash_aref(hash, RAY_SYM("w"));

   if (NIL_P(width) || NIL_P(height))
      rb_raise(rb_eArgError, "Missing parameter: width or height");

   VALUE bitsperpixel = rb_hash_aref(hash, RAY_SYM("bits_per_pixel"));
   if (NIL_P(bitsperpixel)) bitsperpixel = rb_hash_aref(hash, RAY_SYM("bpp"));

   if (NIL_P(bitsperpixel))
      bitsperpixel = INT2FIX(32);

   uint32_t flags = 0;
   if (RTEST(rb_hash_aref(hash, RAY_SYM("sw_surface"))))
      flags |= SDL_SWSURFACE;
   else
      flags |= SDL_HWSURFACE; /* even if hwsurface was false and not nil */

   if (RTEST(rb_hash_aref(hash, RAY_SYM("async_blit"))))
      flags |= SDL_ASYNCBLIT;

   if (!(flags & SDL_SWSURFACE) &&
       RTEST(rb_hash_aref(hash, RAY_SYM("double_buf")))) {
      flags |= SDL_DOUBLEBUF;
   }

   if (RTEST(rb_hash_aref(hash, RAY_SYM("fullscreen"))))
      flags |= SDL_FULLSCREEN;

   if (RTEST(rb_hash_aref(hash, RAY_SYM("no_frame"))))
      flags |= SDL_NOFRAME;

   SDL_Surface *screen = SDL_SetVideoMode(NUM2INT(width),
                                          NUM2INT(height),
                                          NUM2INT(bitsperpixel),
                                          flags);
   if (!screen) {
      rb_raise(rb_eRuntimeError, "Could not create the window (%s)",
               SDL_GetError());
   }

   return ray_create_image(screen);
}

VALUE ray_set_icon(VALUE self, VALUE icon) {
   SDL_WM_SetIcon(ray_rb2surface(icon), NULL);
   return icon;
}

VALUE ray_set_window_title(VALUE self, VALUE title) {
   char *icon = NULL;

   if (!NIL_P(title)) title = rb_String(title);

   SDL_WM_GetCaption(NULL, &icon);
   SDL_WM_SetCaption(NIL_P(title) ? NULL : StringValuePtr(title), icon);

   return title;
}

VALUE ray_set_text_icon(VALUE self, VALUE icon) {
   char *title;
   
   if (!NIL_P(icon)) icon = rb_String(icon);

   SDL_WM_GetCaption(&title, NULL);
   SDL_WM_SetCaption(title, NIL_P(icon) ? NULL : StringValuePtr(icon));

   return icon;
}

VALUE ray_has_image_support(VALUE self) {
#ifdef HAVE_SDL_IMAGE
   return Qtrue;
#else
   return Qfalse;
#endif
}

void Init_ray_ext() {
   ray_mRay = rb_define_module("Ray");

   rb_define_module_function(ray_mRay, "init", ray_init, 0);
   rb_define_module_function(ray_mRay, "stop", ray_stop, 0);

   rb_define_module_function(ray_mRay, "create_window", ray_create_window, 1);
   rb_define_module_function(ray_mRay, "icon=", ray_set_icon, 1);
   rb_define_module_function(ray_mRay, "window_title=", ray_set_window_title, 1);
   rb_define_module_function(ray_mRay, "text_icon=", ray_set_text_icon, 1);

   rb_define_module_function(ray_mRay, "has_image_support?", ray_has_image_support, 0);

   Init_ray_image();
   Init_ray_color();
   Init_ray_rect();
}

int main(int argc, char *argv[]) {
#if defined(HAVE_RUBY_RUN_NODE)
   ruby_init();
   Init_ray_ext();
   ruby_run_node(ruby_options(argc, argv));
#elif defined(HAVE_RUBY_RUN)
   ruby_init();
   Init_ray_ext();
   ruby_init_loadpath();
   ruby_options(argc, argv);
   ruby_run();
#else
   fprintf(stderr, "Please use \"require 'ray'\" on this platform\n");
   return 1;
#endif

   return 0;
}
