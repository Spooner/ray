#include "ray.h"

VALUE ray_cGLBuffer = Qnil;

say_buffer *ray_rb2buffer(VALUE obj) {
  if (!RAY_IS_A(obj, rb_path2class("Ray::GL::Buffer"))) {
    rb_raise(rb_eTypeError, "Can't convert %s into Ray::GL::Buffer",
             RAY_OBJ_CLASSNAME(obj));
  }

  say_buffer * *ptr = NULL;
  Data_Get_Struct(obj, say_buffer*, ptr);

  if (!*ptr)
    rb_raise(rb_eRuntimeError, "trying to use un-initialized buffer");

  return *ptr;
}

static
void ray_buffer_free(say_buffer **ptr) {
  if (*ptr)
    say_buffer_free(*ptr);
}

static
VALUE ray_gl_buffer_alloc(VALUE self) {
  say_buffer **buf = malloc(sizeof(say_buffer*));
  return Data_Wrap_Struct(self, NULL, ray_buffer_free, buf);
}

/*
 * @overload initialize(type, vtype)
 *   @param type  (see Ray::BufferRenderer#initialize)
 *   @param vtype (see Ray::BufferRenderer#initialize)
 *
 *   Creates a new buffer with an arbitrary small size (256).
 */
static
VALUE ray_gl_buffer_init(VALUE self, VALUE type, VALUE vtype) {
  rb_iv_set(self, "@vertex_type", vtype);

  say_buffer **ptr = NULL;
  Data_Get_Struct(self, say_buffer*, ptr);

  *ptr = say_buffer_create(ray_get_vtype(vtype), ray_buf_type(type), 256);

  return self;
}

/* Unbinds any buffer that was bound. */
static
VALUE ray_gl_buffer_unbind(VALUE self) {
  say_buffer_unbind();
  return Qnil;
}

/* Binds the receiver */
static
VALUE ray_gl_buffer_bind(VALUE self) {
  say_buffer_bind(ray_rb2buffer(self));
  return self;
}

/*
 * @overload [](id)
 *   @param [Integer] id
 *   @return [Ray::GL::Vertex, Ray::Vertex] The vertex at the given index.
 */
static
VALUE ray_gl_buffer_get(VALUE self, VALUE i) {
  say_buffer *buf   = ray_rb2buffer(self);
  size_t      size  = say_buffer_get_size(buf);
  size_t      index = NUM2ULONG(i);

  if (index >= size)
    return Qnil;

  VALUE klass  = rb_iv_get(self, "@vertex_type");
  VALUE object = rb_funcall(klass, RAY_METH("allocate"), 0);

  size_t byte_size  = NUM2INT(rb_iv_get(klass, "@vertex_type_size"));

  void *ptr = NULL;
  Data_Get_Struct(object, void, ptr);

  memcpy(ptr, say_buffer_get_vertex(buf, index), byte_size);

  return object;
}

/*
 * @overload []=(id, value)
 *   @param [Integer] id
 *   @param [Ray::GL::Vertex, Ray::Vertex] value The vertex to set the given
 *     index.
 */
static
VALUE ray_gl_buffer_set(VALUE self, VALUE i, VALUE vertex) {
  rb_check_frozen(self);

  VALUE klass = rb_iv_get(self, "@vertex_type");
  if (!RAY_IS_A(vertex, klass)) {
    rb_raise(rb_eTypeError, "Can't convert %s into %s",
             RAY_OBJ_CLASSNAME(vertex), rb_class2name(klass));
  }

  say_buffer *buf   = ray_rb2buffer(self);
  size_t      size  = say_buffer_get_size(buf);
  size_t      index = NUM2ULONG(i);

  if (index >= size) {
    rb_raise(rb_eRangeError, "%zu is outside of range 0...%zu",
             size, index);
  }

  size_t byte_size = NUM2INT(rb_iv_get(klass, "@vertex_type_size"));

  void *ptr = NULL;
  Data_Get_Struct(vertex, void, ptr);

  memcpy(say_buffer_get_vertex(buf, index), ptr, byte_size);

  return vertex;
}

/*
 * @overload update(range = 0...size)
 *   @param [Range<Integer>] range Indices of vertices to update
 *
 *   Updates a part of the buffer.
 *
 * @overload update(first, size)
 *   @param [Integer] first First vertex to update
 *   @param [Integer] size Amount of vertices to update
 */
static
VALUE ray_gl_buffer_update(int argc, VALUE *argv, VALUE self) {
  say_buffer *buf       = ray_rb2buffer(self);
  size_t      max_index = say_buffer_get_size(buf);

  if (argc == 0)
    say_buffer_update(buf);
  else if (argc == 2) {
    size_t begin = NUM2ULONG(argv[0]);
    size_t end   = NUM2ULONG(argv[1]);

    if (end > max_index)
      end = max_index;

    if (begin > end || begin > max_index)
      return self;

    size_t size = (end - begin) + 1;

    say_buffer_update_part(buf, begin, size);
  }
  else {
    VALUE range;
    rb_scan_args(argc, argv, "1", &range); /* raise exception */

    size_t begin = NUM2ULONG(rb_funcall(range, RAY_METH("begin"), 0));
    size_t end   = NUM2ULONG(rb_funcall(range, RAY_METH("end"), 0));

    if (end > max_index)
      end = max_index;

    if (begin > end || begin > max_index)
      return self;

    size_t size = (end - begin) + 1;

    say_buffer_update_part(buf, begin, size);
  }

  return self;
}

/* @return [Integer] Size of the buffer (amount of vertices it contains) */
static
VALUE ray_gl_buffer_size(VALUE self) {
  return ULONG2NUM(say_buffer_get_size(ray_rb2buffer(self)));
}

/*
 * @overload resize(size)
 *   @param [Integere] size New size
 *
 *   Resizes the buffer, alsos causing it to be updated.
 */
static
VALUE ray_gl_buffer_resize(VALUE self, VALUE size) {
  rb_check_frozen(self);
  say_buffer_resize(ray_rb2buffer(self), NUM2ULONG(size));
  return self;
}

/*
 * Document-class: Ray::GL::Buffer
 *
 * Buffers are a low-level way to push vertices onto the GPU so you can draw
 * them. They are used interally by higher level classes, such as
 * Ray::BufferRenderer; Ray also keeps global buffers to store the vertices of
 * drawables.
 *
 * Internally, it uses OpenGL VBOs and VAOs (or just sets vertex attrib pointers
 * if VAOs aren't available).
 */
void Init_ray_gl_buffer() {
  ray_cGLBuffer = rb_define_class_under(ray_mGL, "Buffer", rb_cObject);
  rb_define_alloc_func(ray_cGLBuffer, ray_gl_buffer_alloc);
  rb_define_method(ray_cGLBuffer, "initialize", ray_gl_buffer_init, 2);

  rb_define_singleton_method(ray_cGLBuffer, "unbind", ray_gl_buffer_unbind, 0);
  rb_define_method(ray_cGLBuffer, "bind", ray_gl_buffer_bind, 0);

  rb_define_method(ray_cGLBuffer, "[]",  ray_gl_buffer_get, 1);
  rb_define_method(ray_cGLBuffer, "[]=", ray_gl_buffer_set, 2);

  rb_define_method(ray_cGLBuffer, "update", ray_gl_buffer_update, -1);

  rb_define_method(ray_cGLBuffer, "size", ray_gl_buffer_size, 0);
  rb_define_method(ray_cGLBuffer, "resize", ray_gl_buffer_resize, 1);
}
