#pragma once

namespace mygl
{
    void load(dispatch_table* d);
    void load(dispatch_table* d, loader_function fun);

    dispatch_table::dispatch_table(bool load)
    {
        if(load) mygl::load(this);
    }

    dispatch_table::dispatch_table(loader_function loader)
    {
        mygl::load(this, loader);
    }
    namespace { dispatch_table static_dispatch; }
    dispatch_table& get_static_dispatch() noexcept
    {
        return static_dispatch;
    }
}

void glActiveShaderProgram(std::uint32_t pipeline, std::uint32_t program) noexcept { return mygl::get_static_dispatch().glActiveShaderProgram(pipeline, program); }
void glActiveTexture(GLenum texture) noexcept { return mygl::get_static_dispatch().glActiveTexture(texture); }
void glAttachShader(std::uint32_t program, std::uint32_t shader) noexcept { return mygl::get_static_dispatch().glAttachShader(program, shader); }
void glBeginConditionalRender(std::uint32_t id, GLenum mode) noexcept { return mygl::get_static_dispatch().glBeginConditionalRender(id, mode); }
void glBeginQuery(GLenum target, std::uint32_t id) noexcept { return mygl::get_static_dispatch().glBeginQuery(target, id); }
void glBeginQueryIndexed(GLenum target, std::uint32_t index, std::uint32_t id) noexcept { return mygl::get_static_dispatch().glBeginQueryIndexed(target, index, id); }
void glBeginTransformFeedback(GLenum primitiveMode) noexcept { return mygl::get_static_dispatch().glBeginTransformFeedback(primitiveMode); }
void glBindAttribLocation(std::uint32_t program, std::uint32_t index, const char * name) noexcept { return mygl::get_static_dispatch().glBindAttribLocation(program, index, name); }
void glBindBuffer(GLenum target, std::uint32_t buffer) noexcept { return mygl::get_static_dispatch().glBindBuffer(target, buffer); }
void glBindBufferBase(GLenum target, std::uint32_t index, std::uint32_t buffer) noexcept { return mygl::get_static_dispatch().glBindBufferBase(target, index, buffer); }
void glBindBufferRange(GLenum target, std::uint32_t index, std::uint32_t buffer, std::intptr_t offset, std::int64_t size) noexcept { return mygl::get_static_dispatch().glBindBufferRange(target, index, buffer, offset, size); }
void glBindBuffersBase(GLenum target, std::uint32_t first, std::int32_t count, const std::uint32_t * buffers) noexcept { return mygl::get_static_dispatch().glBindBuffersBase(target, first, count, buffers); }
void glBindBuffersRange(GLenum target, std::uint32_t first, std::int32_t count, const std::uint32_t * buffers, const std::intptr_t * offsets, const std::int64_t * sizes) noexcept { return mygl::get_static_dispatch().glBindBuffersRange(target, first, count, buffers, offsets, sizes); }
void glBindFragDataLocation(std::uint32_t program, std::uint32_t color, const char * name) noexcept { return mygl::get_static_dispatch().glBindFragDataLocation(program, color, name); }
void glBindFragDataLocationIndexed(std::uint32_t program, std::uint32_t colorNumber, std::uint32_t index, const char * name) noexcept { return mygl::get_static_dispatch().glBindFragDataLocationIndexed(program, colorNumber, index, name); }
void glBindFramebuffer(GLenum target, std::uint32_t framebuffer) noexcept { return mygl::get_static_dispatch().glBindFramebuffer(target, framebuffer); }
void glBindImageTexture(std::uint32_t unit, std::uint32_t texture, std::int32_t level, bool layered, std::int32_t layer, GLenum access, GLenum format) noexcept { return mygl::get_static_dispatch().glBindImageTexture(unit, texture, level, layered, layer, access, format); }
void glBindImageTextures(std::uint32_t first, std::int32_t count, const std::uint32_t * textures) noexcept { return mygl::get_static_dispatch().glBindImageTextures(first, count, textures); }
void glBindProgramPipeline(std::uint32_t pipeline) noexcept { return mygl::get_static_dispatch().glBindProgramPipeline(pipeline); }
void glBindRenderbuffer(GLenum target, std::uint32_t renderbuffer) noexcept { return mygl::get_static_dispatch().glBindRenderbuffer(target, renderbuffer); }
void glBindSampler(std::uint32_t unit, std::uint32_t sampler) noexcept { return mygl::get_static_dispatch().glBindSampler(unit, sampler); }
void glBindSamplers(std::uint32_t first, std::int32_t count, const std::uint32_t * samplers) noexcept { return mygl::get_static_dispatch().glBindSamplers(first, count, samplers); }
void glBindTexture(GLenum target, std::uint32_t texture) noexcept { return mygl::get_static_dispatch().glBindTexture(target, texture); }
void glBindTextureUnit(std::uint32_t unit, std::uint32_t texture) noexcept { return mygl::get_static_dispatch().glBindTextureUnit(unit, texture); }
void glBindTextures(std::uint32_t first, std::int32_t count, const std::uint32_t * textures) noexcept { return mygl::get_static_dispatch().glBindTextures(first, count, textures); }
void glBindTransformFeedback(GLenum target, std::uint32_t id) noexcept { return mygl::get_static_dispatch().glBindTransformFeedback(target, id); }
void glBindVertexArray(std::uint32_t array) noexcept { return mygl::get_static_dispatch().glBindVertexArray(array); }
void glBindVertexBuffer(std::uint32_t bindingindex, std::uint32_t buffer, std::intptr_t offset, std::int32_t stride) noexcept { return mygl::get_static_dispatch().glBindVertexBuffer(bindingindex, buffer, offset, stride); }
void glBindVertexBuffers(std::uint32_t first, std::int32_t count, const std::uint32_t * buffers, const std::intptr_t * offsets, const std::int32_t * strides) noexcept { return mygl::get_static_dispatch().glBindVertexBuffers(first, count, buffers, offsets, strides); }
void glBlendColor(float red, float green, float blue, float alpha) noexcept { return mygl::get_static_dispatch().glBlendColor(red, green, blue, alpha); }
void glBlendEquation(GLenum mode) noexcept { return mygl::get_static_dispatch().glBlendEquation(mode); }
void glBlendEquationSeparate(GLenum modeRGB, GLenum modeAlpha) noexcept { return mygl::get_static_dispatch().glBlendEquationSeparate(modeRGB, modeAlpha); }
void glBlendEquationSeparatei(std::uint32_t buf, GLenum modeRGB, GLenum modeAlpha) noexcept { return mygl::get_static_dispatch().glBlendEquationSeparatei(buf, modeRGB, modeAlpha); }
void glBlendEquationSeparateiARB(std::uint32_t buf, GLenum modeRGB, GLenum modeAlpha) noexcept { return mygl::get_static_dispatch().glBlendEquationSeparateiARB(buf, modeRGB, modeAlpha); }
void glBlendEquationi(std::uint32_t buf, GLenum mode) noexcept { return mygl::get_static_dispatch().glBlendEquationi(buf, mode); }
void glBlendEquationiARB(std::uint32_t buf, GLenum mode) noexcept { return mygl::get_static_dispatch().glBlendEquationiARB(buf, mode); }
void glBlendFunc(GLenum sfactor, GLenum dfactor) noexcept { return mygl::get_static_dispatch().glBlendFunc(sfactor, dfactor); }
void glBlendFuncSeparate(GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha) noexcept { return mygl::get_static_dispatch().glBlendFuncSeparate(sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha); }
void glBlendFuncSeparatei(std::uint32_t buf, GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha) noexcept { return mygl::get_static_dispatch().glBlendFuncSeparatei(buf, srcRGB, dstRGB, srcAlpha, dstAlpha); }
void glBlendFuncSeparateiARB(std::uint32_t buf, GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha) noexcept { return mygl::get_static_dispatch().glBlendFuncSeparateiARB(buf, srcRGB, dstRGB, srcAlpha, dstAlpha); }
void glBlendFunci(std::uint32_t buf, GLenum src, GLenum dst) noexcept { return mygl::get_static_dispatch().glBlendFunci(buf, src, dst); }
void glBlendFunciARB(std::uint32_t buf, GLenum src, GLenum dst) noexcept { return mygl::get_static_dispatch().glBlendFunciARB(buf, src, dst); }
void glBlitFramebuffer(std::int32_t srcX0, std::int32_t srcY0, std::int32_t srcX1, std::int32_t srcY1, std::int32_t dstX0, std::int32_t dstY0, std::int32_t dstX1, std::int32_t dstY1, GLbitfield mask, GLenum filter) noexcept { return mygl::get_static_dispatch().glBlitFramebuffer(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter); }
void glBlitNamedFramebuffer(std::uint32_t readFramebuffer, std::uint32_t drawFramebuffer, std::int32_t srcX0, std::int32_t srcY0, std::int32_t srcX1, std::int32_t srcY1, std::int32_t dstX0, std::int32_t dstY0, std::int32_t dstX1, std::int32_t dstY1, GLbitfield mask, GLenum filter) noexcept { return mygl::get_static_dispatch().glBlitNamedFramebuffer(readFramebuffer, drawFramebuffer, srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter); }
void glBufferAddressRangeNV(GLenum pname, std::uint32_t index, std::uint64_t address, std::int64_t length) noexcept { return mygl::get_static_dispatch().glBufferAddressRangeNV(pname, index, address, length); }
void glBufferData(GLenum target, std::int64_t size, const void * data, GLenum usage) noexcept { return mygl::get_static_dispatch().glBufferData(target, size, data, usage); }
void glBufferStorage(GLenum target, std::int64_t size, const void * data, GLbitfield flags) noexcept { return mygl::get_static_dispatch().glBufferStorage(target, size, data, flags); }
void glBufferStorageMemEXT(GLenum target, std::int64_t size, std::uint32_t memory, std::uint64_t offset) noexcept { return mygl::get_static_dispatch().glBufferStorageMemEXT(target, size, memory, offset); }
void glBufferSubData(GLenum target, std::intptr_t offset, std::int64_t size, const void * data) noexcept { return mygl::get_static_dispatch().glBufferSubData(target, offset, size, data); }
void glCallCommandListNV(std::uint32_t list) noexcept { return mygl::get_static_dispatch().glCallCommandListNV(list); }
GLenum glCheckFramebufferStatus(GLenum target) noexcept { return mygl::get_static_dispatch().glCheckFramebufferStatus(target); }
GLenum glCheckNamedFramebufferStatus(std::uint32_t framebuffer, GLenum target) noexcept { return mygl::get_static_dispatch().glCheckNamedFramebufferStatus(framebuffer, target); }
void glClampColor(GLenum target, GLenum clamp) noexcept { return mygl::get_static_dispatch().glClampColor(target, clamp); }
void glClear(GLbitfield mask) noexcept { return mygl::get_static_dispatch().glClear(mask); }
void glClearBufferData(GLenum target, GLenum internalformat, GLenum format, GLenum type, const void * data) noexcept { return mygl::get_static_dispatch().glClearBufferData(target, internalformat, format, type, data); }
void glClearBufferSubData(GLenum target, GLenum internalformat, std::intptr_t offset, std::int64_t size, GLenum format, GLenum type, const void * data) noexcept { return mygl::get_static_dispatch().glClearBufferSubData(target, internalformat, offset, size, format, type, data); }
void glClearBufferfi(GLenum buffer, std::int32_t drawbuffer, float depth, std::int32_t stencil) noexcept { return mygl::get_static_dispatch().glClearBufferfi(buffer, drawbuffer, depth, stencil); }
void glClearBufferfv(GLenum buffer, std::int32_t drawbuffer, const float * value) noexcept { return mygl::get_static_dispatch().glClearBufferfv(buffer, drawbuffer, value); }
void glClearBufferiv(GLenum buffer, std::int32_t drawbuffer, const std::int32_t * value) noexcept { return mygl::get_static_dispatch().glClearBufferiv(buffer, drawbuffer, value); }
void glClearBufferuiv(GLenum buffer, std::int32_t drawbuffer, const std::uint32_t * value) noexcept { return mygl::get_static_dispatch().glClearBufferuiv(buffer, drawbuffer, value); }
void glClearColor(float red, float green, float blue, float alpha) noexcept { return mygl::get_static_dispatch().glClearColor(red, green, blue, alpha); }
void glClearDepth(double depth) noexcept { return mygl::get_static_dispatch().glClearDepth(depth); }
void glClearDepthf(float d) noexcept { return mygl::get_static_dispatch().glClearDepthf(d); }
void glClearNamedBufferData(std::uint32_t buffer, GLenum internalformat, GLenum format, GLenum type, const void * data) noexcept { return mygl::get_static_dispatch().glClearNamedBufferData(buffer, internalformat, format, type, data); }
void glClearNamedBufferSubData(std::uint32_t buffer, GLenum internalformat, std::intptr_t offset, std::int64_t size, GLenum format, GLenum type, const void * data) noexcept { return mygl::get_static_dispatch().glClearNamedBufferSubData(buffer, internalformat, offset, size, format, type, data); }
void glClearNamedFramebufferfi(std::uint32_t framebuffer, GLenum buffer, std::int32_t drawbuffer, float depth, std::int32_t stencil) noexcept { return mygl::get_static_dispatch().glClearNamedFramebufferfi(framebuffer, buffer, drawbuffer, depth, stencil); }
void glClearNamedFramebufferfv(std::uint32_t framebuffer, GLenum buffer, std::int32_t drawbuffer, const float * value) noexcept { return mygl::get_static_dispatch().glClearNamedFramebufferfv(framebuffer, buffer, drawbuffer, value); }
void glClearNamedFramebufferiv(std::uint32_t framebuffer, GLenum buffer, std::int32_t drawbuffer, const std::int32_t * value) noexcept { return mygl::get_static_dispatch().glClearNamedFramebufferiv(framebuffer, buffer, drawbuffer, value); }
void glClearNamedFramebufferuiv(std::uint32_t framebuffer, GLenum buffer, std::int32_t drawbuffer, const std::uint32_t * value) noexcept { return mygl::get_static_dispatch().glClearNamedFramebufferuiv(framebuffer, buffer, drawbuffer, value); }
void glClearStencil(std::int32_t s) noexcept { return mygl::get_static_dispatch().glClearStencil(s); }
void glClearTexImage(std::uint32_t texture, std::int32_t level, GLenum format, GLenum type, const void * data) noexcept { return mygl::get_static_dispatch().glClearTexImage(texture, level, format, type, data); }
void glClearTexSubImage(std::uint32_t texture, std::int32_t level, std::int32_t xoffset, std::int32_t yoffset, std::int32_t zoffset, std::int32_t width, std::int32_t height, std::int32_t depth, GLenum format, GLenum type, const void * data) noexcept { return mygl::get_static_dispatch().glClearTexSubImage(texture, level, xoffset, yoffset, zoffset, width, height, depth, format, type, data); }
GLenum glClientWaitSync(struct __GLsync * sync, GLbitfield flags, std::uint64_t timeout) noexcept { return mygl::get_static_dispatch().glClientWaitSync(sync, flags, timeout); }
void glClipControl(GLenum origin, GLenum depth) noexcept { return mygl::get_static_dispatch().glClipControl(origin, depth); }
void glColorFormatNV(std::int32_t size, GLenum type, std::int32_t stride) noexcept { return mygl::get_static_dispatch().glColorFormatNV(size, type, stride); }
void glColorMask(bool red, bool green, bool blue, bool alpha) noexcept { return mygl::get_static_dispatch().glColorMask(red, green, blue, alpha); }
void glColorMaski(std::uint32_t index, bool r, bool g, bool b, bool a) noexcept { return mygl::get_static_dispatch().glColorMaski(index, r, g, b, a); }
void glColorP3ui(GLenum type, std::uint32_t color) noexcept { return mygl::get_static_dispatch().glColorP3ui(type, color); }
void glColorP3uiv(GLenum type, const std::uint32_t * color) noexcept { return mygl::get_static_dispatch().glColorP3uiv(type, color); }
void glColorP4ui(GLenum type, std::uint32_t color) noexcept { return mygl::get_static_dispatch().glColorP4ui(type, color); }
void glColorP4uiv(GLenum type, const std::uint32_t * color) noexcept { return mygl::get_static_dispatch().glColorP4uiv(type, color); }
void glCommandListSegmentsNV(std::uint32_t list, std::uint32_t segments) noexcept { return mygl::get_static_dispatch().glCommandListSegmentsNV(list, segments); }
void glCompileCommandListNV(std::uint32_t list) noexcept { return mygl::get_static_dispatch().glCompileCommandListNV(list); }
void glCompileShader(std::uint32_t shader) noexcept { return mygl::get_static_dispatch().glCompileShader(shader); }
void glCompressedTexImage1D(GLenum target, std::int32_t level, GLenum internalformat, std::int32_t width, std::int32_t border, std::int32_t imageSize, const void * data) noexcept { return mygl::get_static_dispatch().glCompressedTexImage1D(target, level, internalformat, width, border, imageSize, data); }
void glCompressedTexImage2D(GLenum target, std::int32_t level, GLenum internalformat, std::int32_t width, std::int32_t height, std::int32_t border, std::int32_t imageSize, const void * data) noexcept { return mygl::get_static_dispatch().glCompressedTexImage2D(target, level, internalformat, width, height, border, imageSize, data); }
void glCompressedTexImage3D(GLenum target, std::int32_t level, GLenum internalformat, std::int32_t width, std::int32_t height, std::int32_t depth, std::int32_t border, std::int32_t imageSize, const void * data) noexcept { return mygl::get_static_dispatch().glCompressedTexImage3D(target, level, internalformat, width, height, depth, border, imageSize, data); }
void glCompressedTexSubImage1D(GLenum target, std::int32_t level, std::int32_t xoffset, std::int32_t width, GLenum format, std::int32_t imageSize, const void * data) noexcept { return mygl::get_static_dispatch().glCompressedTexSubImage1D(target, level, xoffset, width, format, imageSize, data); }
void glCompressedTexSubImage2D(GLenum target, std::int32_t level, std::int32_t xoffset, std::int32_t yoffset, std::int32_t width, std::int32_t height, GLenum format, std::int32_t imageSize, const void * data) noexcept { return mygl::get_static_dispatch().glCompressedTexSubImage2D(target, level, xoffset, yoffset, width, height, format, imageSize, data); }
void glCompressedTexSubImage3D(GLenum target, std::int32_t level, std::int32_t xoffset, std::int32_t yoffset, std::int32_t zoffset, std::int32_t width, std::int32_t height, std::int32_t depth, GLenum format, std::int32_t imageSize, const void * data) noexcept { return mygl::get_static_dispatch().glCompressedTexSubImage3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data); }
void glCompressedTextureSubImage1D(std::uint32_t texture, std::int32_t level, std::int32_t xoffset, std::int32_t width, GLenum format, std::int32_t imageSize, const void * data) noexcept { return mygl::get_static_dispatch().glCompressedTextureSubImage1D(texture, level, xoffset, width, format, imageSize, data); }
void glCompressedTextureSubImage2D(std::uint32_t texture, std::int32_t level, std::int32_t xoffset, std::int32_t yoffset, std::int32_t width, std::int32_t height, GLenum format, std::int32_t imageSize, const void * data) noexcept { return mygl::get_static_dispatch().glCompressedTextureSubImage2D(texture, level, xoffset, yoffset, width, height, format, imageSize, data); }
void glCompressedTextureSubImage3D(std::uint32_t texture, std::int32_t level, std::int32_t xoffset, std::int32_t yoffset, std::int32_t zoffset, std::int32_t width, std::int32_t height, std::int32_t depth, GLenum format, std::int32_t imageSize, const void * data) noexcept { return mygl::get_static_dispatch().glCompressedTextureSubImage3D(texture, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data); }
void glCopyBufferSubData(GLenum readTarget, GLenum writeTarget, std::intptr_t readOffset, std::intptr_t writeOffset, std::int64_t size) noexcept { return mygl::get_static_dispatch().glCopyBufferSubData(readTarget, writeTarget, readOffset, writeOffset, size); }
void glCopyImageSubData(std::uint32_t srcName, GLenum srcTarget, std::int32_t srcLevel, std::int32_t srcX, std::int32_t srcY, std::int32_t srcZ, std::uint32_t dstName, GLenum dstTarget, std::int32_t dstLevel, std::int32_t dstX, std::int32_t dstY, std::int32_t dstZ, std::int32_t srcWidth, std::int32_t srcHeight, std::int32_t srcDepth) noexcept { return mygl::get_static_dispatch().glCopyImageSubData(srcName, srcTarget, srcLevel, srcX, srcY, srcZ, dstName, dstTarget, dstLevel, dstX, dstY, dstZ, srcWidth, srcHeight, srcDepth); }
void glCopyNamedBufferSubData(std::uint32_t readBuffer, std::uint32_t writeBuffer, std::intptr_t readOffset, std::intptr_t writeOffset, std::int64_t size) noexcept { return mygl::get_static_dispatch().glCopyNamedBufferSubData(readBuffer, writeBuffer, readOffset, writeOffset, size); }
void glCopyPathNV(std::uint32_t resultPath, std::uint32_t srcPath) noexcept { return mygl::get_static_dispatch().glCopyPathNV(resultPath, srcPath); }
void glCopyTexImage1D(GLenum target, std::int32_t level, GLenum internalformat, std::int32_t x, std::int32_t y, std::int32_t width, std::int32_t border) noexcept { return mygl::get_static_dispatch().glCopyTexImage1D(target, level, internalformat, x, y, width, border); }
void glCopyTexImage2D(GLenum target, std::int32_t level, GLenum internalformat, std::int32_t x, std::int32_t y, std::int32_t width, std::int32_t height, std::int32_t border) noexcept { return mygl::get_static_dispatch().glCopyTexImage2D(target, level, internalformat, x, y, width, height, border); }
void glCopyTexSubImage1D(GLenum target, std::int32_t level, std::int32_t xoffset, std::int32_t x, std::int32_t y, std::int32_t width) noexcept { return mygl::get_static_dispatch().glCopyTexSubImage1D(target, level, xoffset, x, y, width); }
void glCopyTexSubImage2D(GLenum target, std::int32_t level, std::int32_t xoffset, std::int32_t yoffset, std::int32_t x, std::int32_t y, std::int32_t width, std::int32_t height) noexcept { return mygl::get_static_dispatch().glCopyTexSubImage2D(target, level, xoffset, yoffset, x, y, width, height); }
void glCopyTexSubImage3D(GLenum target, std::int32_t level, std::int32_t xoffset, std::int32_t yoffset, std::int32_t zoffset, std::int32_t x, std::int32_t y, std::int32_t width, std::int32_t height) noexcept { return mygl::get_static_dispatch().glCopyTexSubImage3D(target, level, xoffset, yoffset, zoffset, x, y, width, height); }
void glCopyTextureSubImage1D(std::uint32_t texture, std::int32_t level, std::int32_t xoffset, std::int32_t x, std::int32_t y, std::int32_t width) noexcept { return mygl::get_static_dispatch().glCopyTextureSubImage1D(texture, level, xoffset, x, y, width); }
void glCopyTextureSubImage2D(std::uint32_t texture, std::int32_t level, std::int32_t xoffset, std::int32_t yoffset, std::int32_t x, std::int32_t y, std::int32_t width, std::int32_t height) noexcept { return mygl::get_static_dispatch().glCopyTextureSubImage2D(texture, level, xoffset, yoffset, x, y, width, height); }
void glCopyTextureSubImage3D(std::uint32_t texture, std::int32_t level, std::int32_t xoffset, std::int32_t yoffset, std::int32_t zoffset, std::int32_t x, std::int32_t y, std::int32_t width, std::int32_t height) noexcept { return mygl::get_static_dispatch().glCopyTextureSubImage3D(texture, level, xoffset, yoffset, zoffset, x, y, width, height); }
void glCoverFillPathInstancedNV(std::int32_t numPaths, GLenum pathNameType, const void * paths, std::uint32_t pathBase, GLenum coverMode, GLenum transformType, const float * transformValues) noexcept { return mygl::get_static_dispatch().glCoverFillPathInstancedNV(numPaths, pathNameType, paths, pathBase, coverMode, transformType, transformValues); }
void glCoverFillPathNV(std::uint32_t path, GLenum coverMode) noexcept { return mygl::get_static_dispatch().glCoverFillPathNV(path, coverMode); }
void glCoverStrokePathInstancedNV(std::int32_t numPaths, GLenum pathNameType, const void * paths, std::uint32_t pathBase, GLenum coverMode, GLenum transformType, const float * transformValues) noexcept { return mygl::get_static_dispatch().glCoverStrokePathInstancedNV(numPaths, pathNameType, paths, pathBase, coverMode, transformType, transformValues); }
void glCoverStrokePathNV(std::uint32_t path, GLenum coverMode) noexcept { return mygl::get_static_dispatch().glCoverStrokePathNV(path, coverMode); }
void glCreateBuffers(std::int32_t n, std::uint32_t * buffers) noexcept { return mygl::get_static_dispatch().glCreateBuffers(n, buffers); }
void glCreateCommandListsNV(std::int32_t n, std::uint32_t * lists) noexcept { return mygl::get_static_dispatch().glCreateCommandListsNV(n, lists); }
void glCreateFramebuffers(std::int32_t n, std::uint32_t * framebuffers) noexcept { return mygl::get_static_dispatch().glCreateFramebuffers(n, framebuffers); }
void glCreateMemoryObjectsEXT(std::int32_t n, std::uint32_t * memoryObjects) noexcept { return mygl::get_static_dispatch().glCreateMemoryObjectsEXT(n, memoryObjects); }
std::uint32_t glCreateProgram() noexcept { return mygl::get_static_dispatch().glCreateProgram(); }
void glCreateProgramPipelines(std::int32_t n, std::uint32_t * pipelines) noexcept { return mygl::get_static_dispatch().glCreateProgramPipelines(n, pipelines); }
void glCreateQueries(GLenum target, std::int32_t n, std::uint32_t * ids) noexcept { return mygl::get_static_dispatch().glCreateQueries(target, n, ids); }
void glCreateRenderbuffers(std::int32_t n, std::uint32_t * renderbuffers) noexcept { return mygl::get_static_dispatch().glCreateRenderbuffers(n, renderbuffers); }
void glCreateSamplers(std::int32_t n, std::uint32_t * samplers) noexcept { return mygl::get_static_dispatch().glCreateSamplers(n, samplers); }
std::uint32_t glCreateShader(GLenum type) noexcept { return mygl::get_static_dispatch().glCreateShader(type); }
std::uint32_t glCreateShaderProgramv(GLenum type, std::int32_t count, const char *const* strings) noexcept { return mygl::get_static_dispatch().glCreateShaderProgramv(type, count, strings); }
void glCreateStatesNV(std::int32_t n, std::uint32_t * states) noexcept { return mygl::get_static_dispatch().glCreateStatesNV(n, states); }
void glCreateTextures(GLenum target, std::int32_t n, std::uint32_t * textures) noexcept { return mygl::get_static_dispatch().glCreateTextures(target, n, textures); }
void glCreateTransformFeedbacks(std::int32_t n, std::uint32_t * ids) noexcept { return mygl::get_static_dispatch().glCreateTransformFeedbacks(n, ids); }
void glCreateVertexArrays(std::int32_t n, std::uint32_t * arrays) noexcept { return mygl::get_static_dispatch().glCreateVertexArrays(n, arrays); }
void glCullFace(GLenum mode) noexcept { return mygl::get_static_dispatch().glCullFace(mode); }
void glDebugMessageCallback(GLDEBUGPROC callback, const void * userParam) noexcept { return mygl::get_static_dispatch().glDebugMessageCallback(callback, userParam); }
void glDebugMessageCallbackKHR(GLDEBUGPROCKHR callback, const void * userParam) noexcept { return mygl::get_static_dispatch().glDebugMessageCallbackKHR(callback, userParam); }
void glDebugMessageControl(GLenum source, GLenum type, GLenum severity, std::int32_t count, const std::uint32_t * ids, bool enabled) noexcept { return mygl::get_static_dispatch().glDebugMessageControl(source, type, severity, count, ids, enabled); }
void glDebugMessageControlKHR(GLenum source, GLenum type, GLenum severity, std::int32_t count, const std::uint32_t * ids, bool enabled) noexcept { return mygl::get_static_dispatch().glDebugMessageControlKHR(source, type, severity, count, ids, enabled); }
void glDebugMessageInsert(GLenum source, GLenum type, std::uint32_t id, GLenum severity, std::int32_t length, const char * buf) noexcept { return mygl::get_static_dispatch().glDebugMessageInsert(source, type, id, severity, length, buf); }
void glDebugMessageInsertKHR(GLenum source, GLenum type, std::uint32_t id, GLenum severity, std::int32_t length, const char * buf) noexcept { return mygl::get_static_dispatch().glDebugMessageInsertKHR(source, type, id, severity, length, buf); }
void glDeleteBuffers(std::int32_t n, const std::uint32_t * buffers) noexcept { return mygl::get_static_dispatch().glDeleteBuffers(n, buffers); }
void glDeleteCommandListsNV(std::int32_t n, const std::uint32_t * lists) noexcept { return mygl::get_static_dispatch().glDeleteCommandListsNV(n, lists); }
void glDeleteFramebuffers(std::int32_t n, const std::uint32_t * framebuffers) noexcept { return mygl::get_static_dispatch().glDeleteFramebuffers(n, framebuffers); }
void glDeleteMemoryObjectsEXT(std::int32_t n, const std::uint32_t * memoryObjects) noexcept { return mygl::get_static_dispatch().glDeleteMemoryObjectsEXT(n, memoryObjects); }
void glDeletePathsNV(std::uint32_t path, std::int32_t range) noexcept { return mygl::get_static_dispatch().glDeletePathsNV(path, range); }
void glDeleteProgram(std::uint32_t program) noexcept { return mygl::get_static_dispatch().glDeleteProgram(program); }
void glDeleteProgramPipelines(std::int32_t n, const std::uint32_t * pipelines) noexcept { return mygl::get_static_dispatch().glDeleteProgramPipelines(n, pipelines); }
void glDeleteQueries(std::int32_t n, const std::uint32_t * ids) noexcept { return mygl::get_static_dispatch().glDeleteQueries(n, ids); }
void glDeleteRenderbuffers(std::int32_t n, const std::uint32_t * renderbuffers) noexcept { return mygl::get_static_dispatch().glDeleteRenderbuffers(n, renderbuffers); }
void glDeleteSamplers(std::int32_t count, const std::uint32_t * samplers) noexcept { return mygl::get_static_dispatch().glDeleteSamplers(count, samplers); }
void glDeleteSemaphoresEXT(std::int32_t n, const std::uint32_t * semaphores) noexcept { return mygl::get_static_dispatch().glDeleteSemaphoresEXT(n, semaphores); }
void glDeleteShader(std::uint32_t shader) noexcept { return mygl::get_static_dispatch().glDeleteShader(shader); }
void glDeleteStatesNV(std::int32_t n, const std::uint32_t * states) noexcept { return mygl::get_static_dispatch().glDeleteStatesNV(n, states); }
void glDeleteSync(struct __GLsync * sync) noexcept { return mygl::get_static_dispatch().glDeleteSync(sync); }
void glDeleteTextures(std::int32_t n, const std::uint32_t * textures) noexcept { return mygl::get_static_dispatch().glDeleteTextures(n, textures); }
void glDeleteTransformFeedbacks(std::int32_t n, const std::uint32_t * ids) noexcept { return mygl::get_static_dispatch().glDeleteTransformFeedbacks(n, ids); }
void glDeleteVertexArrays(std::int32_t n, const std::uint32_t * arrays) noexcept { return mygl::get_static_dispatch().glDeleteVertexArrays(n, arrays); }
void glDepthBoundsEXT(double zmin, double zmax) noexcept { return mygl::get_static_dispatch().glDepthBoundsEXT(zmin, zmax); }
void glDepthFunc(GLenum func) noexcept { return mygl::get_static_dispatch().glDepthFunc(func); }
void glDepthMask(bool flag) noexcept { return mygl::get_static_dispatch().glDepthMask(flag); }
void glDepthRange(double n, double f) noexcept { return mygl::get_static_dispatch().glDepthRange(n, f); }
void glDepthRangeArraydvNV(std::uint32_t first, std::int32_t count, const double * v) noexcept { return mygl::get_static_dispatch().glDepthRangeArraydvNV(first, count, v); }
void glDepthRangeArrayv(std::uint32_t first, std::int32_t count, const double * v) noexcept { return mygl::get_static_dispatch().glDepthRangeArrayv(first, count, v); }
void glDepthRangeIndexed(std::uint32_t index, double n, double f) noexcept { return mygl::get_static_dispatch().glDepthRangeIndexed(index, n, f); }
void glDepthRangeIndexeddNV(std::uint32_t index, double n, double f) noexcept { return mygl::get_static_dispatch().glDepthRangeIndexeddNV(index, n, f); }
void glDepthRangef(float n, float f) noexcept { return mygl::get_static_dispatch().glDepthRangef(n, f); }
void glDetachShader(std::uint32_t program, std::uint32_t shader) noexcept { return mygl::get_static_dispatch().glDetachShader(program, shader); }
void glDisable(GLenum cap) noexcept { return mygl::get_static_dispatch().glDisable(cap); }
void glDisableClientState(GLenum array) noexcept { return mygl::get_static_dispatch().glDisableClientState(array); }
void glDisableVertexArrayAttrib(std::uint32_t vaobj, std::uint32_t index) noexcept { return mygl::get_static_dispatch().glDisableVertexArrayAttrib(vaobj, index); }
void glDisableVertexAttribArray(std::uint32_t index) noexcept { return mygl::get_static_dispatch().glDisableVertexAttribArray(index); }
void glDisablei(GLenum target, std::uint32_t index) noexcept { return mygl::get_static_dispatch().glDisablei(target, index); }
void glDispatchCompute(std::uint32_t num_groups_x, std::uint32_t num_groups_y, std::uint32_t num_groups_z) noexcept { return mygl::get_static_dispatch().glDispatchCompute(num_groups_x, num_groups_y, num_groups_z); }
void glDispatchComputeGroupSizeARB(std::uint32_t num_groups_x, std::uint32_t num_groups_y, std::uint32_t num_groups_z, std::uint32_t group_size_x, std::uint32_t group_size_y, std::uint32_t group_size_z) noexcept { return mygl::get_static_dispatch().glDispatchComputeGroupSizeARB(num_groups_x, num_groups_y, num_groups_z, group_size_x, group_size_y, group_size_z); }
void glDispatchComputeIndirect(std::intptr_t indirect) noexcept { return mygl::get_static_dispatch().glDispatchComputeIndirect(indirect); }
void glDrawArrays(GLenum mode, std::int32_t first, std::int32_t count) noexcept { return mygl::get_static_dispatch().glDrawArrays(mode, first, count); }
void glDrawArraysIndirect(GLenum mode, const void * indirect) noexcept { return mygl::get_static_dispatch().glDrawArraysIndirect(mode, indirect); }
void glDrawArraysInstanced(GLenum mode, std::int32_t first, std::int32_t count, std::int32_t instancecount) noexcept { return mygl::get_static_dispatch().glDrawArraysInstanced(mode, first, count, instancecount); }
void glDrawArraysInstancedBaseInstance(GLenum mode, std::int32_t first, std::int32_t count, std::int32_t instancecount, std::uint32_t baseinstance) noexcept { return mygl::get_static_dispatch().glDrawArraysInstancedBaseInstance(mode, first, count, instancecount, baseinstance); }
void glDrawBuffer(GLenum buf) noexcept { return mygl::get_static_dispatch().glDrawBuffer(buf); }
void glDrawBuffers(std::int32_t n, const GLenum * bufs) noexcept { return mygl::get_static_dispatch().glDrawBuffers(n, bufs); }
void glDrawCommandsAddressNV(GLenum primitiveMode, const std::uint64_t * indirects, const std::int32_t * sizes, std::uint32_t count) noexcept { return mygl::get_static_dispatch().glDrawCommandsAddressNV(primitiveMode, indirects, sizes, count); }
void glDrawCommandsNV(GLenum primitiveMode, std::uint32_t buffer, const std::intptr_t * indirects, const std::int32_t * sizes, std::uint32_t count) noexcept { return mygl::get_static_dispatch().glDrawCommandsNV(primitiveMode, buffer, indirects, sizes, count); }
void glDrawCommandsStatesAddressNV(const std::uint64_t * indirects, const std::int32_t * sizes, const std::uint32_t * states, const std::uint32_t * fbos, std::uint32_t count) noexcept { return mygl::get_static_dispatch().glDrawCommandsStatesAddressNV(indirects, sizes, states, fbos, count); }
void glDrawCommandsStatesNV(std::uint32_t buffer, const std::intptr_t * indirects, const std::int32_t * sizes, const std::uint32_t * states, const std::uint32_t * fbos, std::uint32_t count) noexcept { return mygl::get_static_dispatch().glDrawCommandsStatesNV(buffer, indirects, sizes, states, fbos, count); }
void glDrawElements(GLenum mode, std::int32_t count, GLenum type, const void * indices) noexcept { return mygl::get_static_dispatch().glDrawElements(mode, count, type, indices); }
void glDrawElementsBaseVertex(GLenum mode, std::int32_t count, GLenum type, const void * indices, std::int32_t basevertex) noexcept { return mygl::get_static_dispatch().glDrawElementsBaseVertex(mode, count, type, indices, basevertex); }
void glDrawElementsIndirect(GLenum mode, GLenum type, const void * indirect) noexcept { return mygl::get_static_dispatch().glDrawElementsIndirect(mode, type, indirect); }
void glDrawElementsInstanced(GLenum mode, std::int32_t count, GLenum type, const void * indices, std::int32_t instancecount) noexcept { return mygl::get_static_dispatch().glDrawElementsInstanced(mode, count, type, indices, instancecount); }
void glDrawElementsInstancedBaseInstance(GLenum mode, std::int32_t count, GLenum type, const void * indices, std::int32_t instancecount, std::uint32_t baseinstance) noexcept { return mygl::get_static_dispatch().glDrawElementsInstancedBaseInstance(mode, count, type, indices, instancecount, baseinstance); }
void glDrawElementsInstancedBaseVertex(GLenum mode, std::int32_t count, GLenum type, const void * indices, std::int32_t instancecount, std::int32_t basevertex) noexcept { return mygl::get_static_dispatch().glDrawElementsInstancedBaseVertex(mode, count, type, indices, instancecount, basevertex); }
void glDrawElementsInstancedBaseVertexBaseInstance(GLenum mode, std::int32_t count, GLenum type, const void * indices, std::int32_t instancecount, std::int32_t basevertex, std::uint32_t baseinstance) noexcept { return mygl::get_static_dispatch().glDrawElementsInstancedBaseVertexBaseInstance(mode, count, type, indices, instancecount, basevertex, baseinstance); }
void glDrawRangeElements(GLenum mode, std::uint32_t start, std::uint32_t end, std::int32_t count, GLenum type, const void * indices) noexcept { return mygl::get_static_dispatch().glDrawRangeElements(mode, start, end, count, type, indices); }
void glDrawRangeElementsBaseVertex(GLenum mode, std::uint32_t start, std::uint32_t end, std::int32_t count, GLenum type, const void * indices, std::int32_t basevertex) noexcept { return mygl::get_static_dispatch().glDrawRangeElementsBaseVertex(mode, start, end, count, type, indices, basevertex); }
void glDrawTextureNV(std::uint32_t texture, std::uint32_t sampler, float x0, float y0, float x1, float y1, float z, float s0, float t0, float s1, float t1) noexcept { return mygl::get_static_dispatch().glDrawTextureNV(texture, sampler, x0, y0, x1, y1, z, s0, t0, s1, t1); }
void glDrawTransformFeedback(GLenum mode, std::uint32_t id) noexcept { return mygl::get_static_dispatch().glDrawTransformFeedback(mode, id); }
void glDrawTransformFeedbackInstanced(GLenum mode, std::uint32_t id, std::int32_t instancecount) noexcept { return mygl::get_static_dispatch().glDrawTransformFeedbackInstanced(mode, id, instancecount); }
void glDrawTransformFeedbackStream(GLenum mode, std::uint32_t id, std::uint32_t stream) noexcept { return mygl::get_static_dispatch().glDrawTransformFeedbackStream(mode, id, stream); }
void glDrawTransformFeedbackStreamInstanced(GLenum mode, std::uint32_t id, std::uint32_t stream, std::int32_t instancecount) noexcept { return mygl::get_static_dispatch().glDrawTransformFeedbackStreamInstanced(mode, id, stream, instancecount); }
void glEdgeFlagFormatNV(std::int32_t stride) noexcept { return mygl::get_static_dispatch().glEdgeFlagFormatNV(stride); }
void glEnable(GLenum cap) noexcept { return mygl::get_static_dispatch().glEnable(cap); }
void glEnableClientState(GLenum array) noexcept { return mygl::get_static_dispatch().glEnableClientState(array); }
void glEnableVertexArrayAttrib(std::uint32_t vaobj, std::uint32_t index) noexcept { return mygl::get_static_dispatch().glEnableVertexArrayAttrib(vaobj, index); }
void glEnableVertexAttribArray(std::uint32_t index) noexcept { return mygl::get_static_dispatch().glEnableVertexAttribArray(index); }
void glEnablei(GLenum target, std::uint32_t index) noexcept { return mygl::get_static_dispatch().glEnablei(target, index); }
void glEndConditionalRender() noexcept { return mygl::get_static_dispatch().glEndConditionalRender(); }
void glEndQuery(GLenum target) noexcept { return mygl::get_static_dispatch().glEndQuery(target); }
void glEndQueryIndexed(GLenum target, std::uint32_t index) noexcept { return mygl::get_static_dispatch().glEndQueryIndexed(target, index); }
void glEndTransformFeedback() noexcept { return mygl::get_static_dispatch().glEndTransformFeedback(); }
struct __GLsync * glFenceSync(GLenum condition, GLbitfield flags) noexcept { return mygl::get_static_dispatch().glFenceSync(condition, flags); }
void glFinish() noexcept { return mygl::get_static_dispatch().glFinish(); }
void glFlush() noexcept { return mygl::get_static_dispatch().glFlush(); }
void glFlushMappedBufferRange(GLenum target, std::intptr_t offset, std::int64_t length) noexcept { return mygl::get_static_dispatch().glFlushMappedBufferRange(target, offset, length); }
void glFlushMappedNamedBufferRange(std::uint32_t buffer, std::intptr_t offset, std::int64_t length) noexcept { return mygl::get_static_dispatch().glFlushMappedNamedBufferRange(buffer, offset, length); }
void glFogCoordFormatNV(GLenum type, std::int32_t stride) noexcept { return mygl::get_static_dispatch().glFogCoordFormatNV(type, stride); }
void glFramebufferParameteri(GLenum target, GLenum pname, std::int32_t param) noexcept { return mygl::get_static_dispatch().glFramebufferParameteri(target, pname, param); }
void glFramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, std::uint32_t renderbuffer) noexcept { return mygl::get_static_dispatch().glFramebufferRenderbuffer(target, attachment, renderbuffertarget, renderbuffer); }
void glFramebufferTexture(GLenum target, GLenum attachment, std::uint32_t texture, std::int32_t level) noexcept { return mygl::get_static_dispatch().glFramebufferTexture(target, attachment, texture, level); }
void glFramebufferTexture1D(GLenum target, GLenum attachment, GLenum textarget, std::uint32_t texture, std::int32_t level) noexcept { return mygl::get_static_dispatch().glFramebufferTexture1D(target, attachment, textarget, texture, level); }
void glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, std::uint32_t texture, std::int32_t level) noexcept { return mygl::get_static_dispatch().glFramebufferTexture2D(target, attachment, textarget, texture, level); }
void glFramebufferTexture3D(GLenum target, GLenum attachment, GLenum textarget, std::uint32_t texture, std::int32_t level, std::int32_t zoffset) noexcept { return mygl::get_static_dispatch().glFramebufferTexture3D(target, attachment, textarget, texture, level, zoffset); }
void glFramebufferTextureLayer(GLenum target, GLenum attachment, std::uint32_t texture, std::int32_t level, std::int32_t layer) noexcept { return mygl::get_static_dispatch().glFramebufferTextureLayer(target, attachment, texture, level, layer); }
void glFrontFace(GLenum mode) noexcept { return mygl::get_static_dispatch().glFrontFace(mode); }
void glGenBuffers(std::int32_t n, std::uint32_t * buffers) noexcept { return mygl::get_static_dispatch().glGenBuffers(n, buffers); }
void glGenFramebuffers(std::int32_t n, std::uint32_t * framebuffers) noexcept { return mygl::get_static_dispatch().glGenFramebuffers(n, framebuffers); }
std::uint32_t glGenPathsNV(std::int32_t range) noexcept { return mygl::get_static_dispatch().glGenPathsNV(range); }
void glGenProgramPipelines(std::int32_t n, std::uint32_t * pipelines) noexcept { return mygl::get_static_dispatch().glGenProgramPipelines(n, pipelines); }
void glGenQueries(std::int32_t n, std::uint32_t * ids) noexcept { return mygl::get_static_dispatch().glGenQueries(n, ids); }
void glGenRenderbuffers(std::int32_t n, std::uint32_t * renderbuffers) noexcept { return mygl::get_static_dispatch().glGenRenderbuffers(n, renderbuffers); }
void glGenSamplers(std::int32_t count, std::uint32_t * samplers) noexcept { return mygl::get_static_dispatch().glGenSamplers(count, samplers); }
void glGenSemaphoresEXT(std::int32_t n, std::uint32_t * semaphores) noexcept { return mygl::get_static_dispatch().glGenSemaphoresEXT(n, semaphores); }
void glGenTextures(std::int32_t n, std::uint32_t * textures) noexcept { return mygl::get_static_dispatch().glGenTextures(n, textures); }
void glGenTransformFeedbacks(std::int32_t n, std::uint32_t * ids) noexcept { return mygl::get_static_dispatch().glGenTransformFeedbacks(n, ids); }
void glGenVertexArrays(std::int32_t n, std::uint32_t * arrays) noexcept { return mygl::get_static_dispatch().glGenVertexArrays(n, arrays); }
void glGenerateMipmap(GLenum target) noexcept { return mygl::get_static_dispatch().glGenerateMipmap(target); }
void glGenerateTextureMipmap(std::uint32_t texture) noexcept { return mygl::get_static_dispatch().glGenerateTextureMipmap(texture); }
void glGetActiveAtomicCounterBufferiv(std::uint32_t program, std::uint32_t bufferIndex, GLenum pname, std::int32_t * params) noexcept { return mygl::get_static_dispatch().glGetActiveAtomicCounterBufferiv(program, bufferIndex, pname, params); }
void glGetActiveAttrib(std::uint32_t program, std::uint32_t index, std::int32_t bufSize, std::int32_t * length, std::int32_t * size, GLenum * type, char * name) noexcept { return mygl::get_static_dispatch().glGetActiveAttrib(program, index, bufSize, length, size, type, name); }
void glGetActiveSubroutineName(std::uint32_t program, GLenum shadertype, std::uint32_t index, std::int32_t bufSize, std::int32_t * length, char * name) noexcept { return mygl::get_static_dispatch().glGetActiveSubroutineName(program, shadertype, index, bufSize, length, name); }
void glGetActiveSubroutineUniformName(std::uint32_t program, GLenum shadertype, std::uint32_t index, std::int32_t bufSize, std::int32_t * length, char * name) noexcept { return mygl::get_static_dispatch().glGetActiveSubroutineUniformName(program, shadertype, index, bufSize, length, name); }
void glGetActiveSubroutineUniformiv(std::uint32_t program, GLenum shadertype, std::uint32_t index, GLenum pname, std::int32_t * values) noexcept { return mygl::get_static_dispatch().glGetActiveSubroutineUniformiv(program, shadertype, index, pname, values); }
void glGetActiveUniform(std::uint32_t program, std::uint32_t index, std::int32_t bufSize, std::int32_t * length, std::int32_t * size, GLenum * type, char * name) noexcept { return mygl::get_static_dispatch().glGetActiveUniform(program, index, bufSize, length, size, type, name); }
void glGetActiveUniformBlockName(std::uint32_t program, std::uint32_t uniformBlockIndex, std::int32_t bufSize, std::int32_t * length, char * uniformBlockName) noexcept { return mygl::get_static_dispatch().glGetActiveUniformBlockName(program, uniformBlockIndex, bufSize, length, uniformBlockName); }
void glGetActiveUniformBlockiv(std::uint32_t program, std::uint32_t uniformBlockIndex, GLenum pname, std::int32_t * params) noexcept { return mygl::get_static_dispatch().glGetActiveUniformBlockiv(program, uniformBlockIndex, pname, params); }
void glGetActiveUniformName(std::uint32_t program, std::uint32_t uniformIndex, std::int32_t bufSize, std::int32_t * length, char * uniformName) noexcept { return mygl::get_static_dispatch().glGetActiveUniformName(program, uniformIndex, bufSize, length, uniformName); }
void glGetActiveUniformsiv(std::uint32_t program, std::int32_t uniformCount, const std::uint32_t * uniformIndices, GLenum pname, std::int32_t * params) noexcept { return mygl::get_static_dispatch().glGetActiveUniformsiv(program, uniformCount, uniformIndices, pname, params); }
void glGetAttachedShaders(std::uint32_t program, std::int32_t maxCount, std::int32_t * count, std::uint32_t * shaders) noexcept { return mygl::get_static_dispatch().glGetAttachedShaders(program, maxCount, count, shaders); }
std::int32_t glGetAttribLocation(std::uint32_t program, const char * name) noexcept { return mygl::get_static_dispatch().glGetAttribLocation(program, name); }
void glGetBooleani_v(GLenum target, std::uint32_t index, bool * data) noexcept { return mygl::get_static_dispatch().glGetBooleani_v(target, index, data); }
void glGetBooleanv(GLenum pname, bool * data) noexcept { return mygl::get_static_dispatch().glGetBooleanv(pname, data); }
void glGetBufferParameteri64v(GLenum target, GLenum pname, std::int64_t * params) noexcept { return mygl::get_static_dispatch().glGetBufferParameteri64v(target, pname, params); }
void glGetBufferParameteriv(GLenum target, GLenum pname, std::int32_t * params) noexcept { return mygl::get_static_dispatch().glGetBufferParameteriv(target, pname, params); }
void glGetBufferParameterui64vNV(GLenum target, GLenum pname, std::uint64_t * params) noexcept { return mygl::get_static_dispatch().glGetBufferParameterui64vNV(target, pname, params); }
void glGetBufferPointerv(GLenum target, GLenum pname, void ** params) noexcept { return mygl::get_static_dispatch().glGetBufferPointerv(target, pname, params); }
void glGetBufferSubData(GLenum target, std::intptr_t offset, std::int64_t size, void * data) noexcept { return mygl::get_static_dispatch().glGetBufferSubData(target, offset, size, data); }
std::uint32_t glGetCommandHeaderNV(GLenum tokenID, std::uint32_t size) noexcept { return mygl::get_static_dispatch().glGetCommandHeaderNV(tokenID, size); }
void glGetCompressedTexImage(GLenum target, std::int32_t level, void * img) noexcept { return mygl::get_static_dispatch().glGetCompressedTexImage(target, level, img); }
void glGetCompressedTextureImage(std::uint32_t texture, std::int32_t level, std::int32_t bufSize, void * pixels) noexcept { return mygl::get_static_dispatch().glGetCompressedTextureImage(texture, level, bufSize, pixels); }
void glGetCompressedTextureSubImage(std::uint32_t texture, std::int32_t level, std::int32_t xoffset, std::int32_t yoffset, std::int32_t zoffset, std::int32_t width, std::int32_t height, std::int32_t depth, std::int32_t bufSize, void * pixels) noexcept { return mygl::get_static_dispatch().glGetCompressedTextureSubImage(texture, level, xoffset, yoffset, zoffset, width, height, depth, bufSize, pixels); }
std::uint32_t glGetDebugMessageLog(std::uint32_t count, std::int32_t bufSize, GLenum * sources, GLenum * types, std::uint32_t * ids, GLenum * severities, std::int32_t * lengths, char * messageLog) noexcept { return mygl::get_static_dispatch().glGetDebugMessageLog(count, bufSize, sources, types, ids, severities, lengths, messageLog); }
std::uint32_t glGetDebugMessageLogKHR(std::uint32_t count, std::int32_t bufSize, GLenum * sources, GLenum * types, std::uint32_t * ids, GLenum * severities, std::int32_t * lengths, char * messageLog) noexcept { return mygl::get_static_dispatch().glGetDebugMessageLogKHR(count, bufSize, sources, types, ids, severities, lengths, messageLog); }
void glGetDoublei_v(GLenum target, std::uint32_t index, double * data) noexcept { return mygl::get_static_dispatch().glGetDoublei_v(target, index, data); }
void glGetDoublev(GLenum pname, double * data) noexcept { return mygl::get_static_dispatch().glGetDoublev(pname, data); }
GLenum glGetError() noexcept { return mygl::get_static_dispatch().glGetError(); }
void glGetFloati_v(GLenum target, std::uint32_t index, float * data) noexcept { return mygl::get_static_dispatch().glGetFloati_v(target, index, data); }
void glGetFloatv(GLenum pname, float * data) noexcept { return mygl::get_static_dispatch().glGetFloatv(pname, data); }
std::int32_t glGetFragDataIndex(std::uint32_t program, const char * name) noexcept { return mygl::get_static_dispatch().glGetFragDataIndex(program, name); }
std::int32_t glGetFragDataLocation(std::uint32_t program, const char * name) noexcept { return mygl::get_static_dispatch().glGetFragDataLocation(program, name); }
void glGetFramebufferAttachmentParameteriv(GLenum target, GLenum attachment, GLenum pname, std::int32_t * params) noexcept { return mygl::get_static_dispatch().glGetFramebufferAttachmentParameteriv(target, attachment, pname, params); }
void glGetFramebufferParameteriv(GLenum target, GLenum pname, std::int32_t * params) noexcept { return mygl::get_static_dispatch().glGetFramebufferParameteriv(target, pname, params); }
GLenum glGetGraphicsResetStatus() noexcept { return mygl::get_static_dispatch().glGetGraphicsResetStatus(); }
std::uint64_t glGetImageHandleARB(std::uint32_t texture, std::int32_t level, bool layered, std::int32_t layer, GLenum format) noexcept { return mygl::get_static_dispatch().glGetImageHandleARB(texture, level, layered, layer, format); }
void glGetInteger64i_v(GLenum target, std::uint32_t index, std::int64_t * data) noexcept { return mygl::get_static_dispatch().glGetInteger64i_v(target, index, data); }
void glGetInteger64v(GLenum pname, std::int64_t * data) noexcept { return mygl::get_static_dispatch().glGetInteger64v(pname, data); }
void glGetIntegeri_v(GLenum target, std::uint32_t index, std::int32_t * data) noexcept { return mygl::get_static_dispatch().glGetIntegeri_v(target, index, data); }
void glGetIntegerui64i_vNV(GLenum value, std::uint32_t index, std::uint64_t * result) noexcept { return mygl::get_static_dispatch().glGetIntegerui64i_vNV(value, index, result); }
void glGetIntegerui64vNV(GLenum value, std::uint64_t * result) noexcept { return mygl::get_static_dispatch().glGetIntegerui64vNV(value, result); }
void glGetIntegerv(GLenum pname, std::int32_t * data) noexcept { return mygl::get_static_dispatch().glGetIntegerv(pname, data); }
void glGetInternalformati64v(GLenum target, GLenum internalformat, GLenum pname, std::int32_t count, std::int64_t * params) noexcept { return mygl::get_static_dispatch().glGetInternalformati64v(target, internalformat, pname, count, params); }
void glGetInternalformativ(GLenum target, GLenum internalformat, GLenum pname, std::int32_t count, std::int32_t * params) noexcept { return mygl::get_static_dispatch().glGetInternalformativ(target, internalformat, pname, count, params); }
void glGetMemoryObjectParameterivEXT(std::uint32_t memoryObject, GLenum pname, std::int32_t * params) noexcept { return mygl::get_static_dispatch().glGetMemoryObjectParameterivEXT(memoryObject, pname, params); }
void glGetMultisamplefv(GLenum pname, std::uint32_t index, float * val) noexcept { return mygl::get_static_dispatch().glGetMultisamplefv(pname, index, val); }
void glGetNamedBufferParameteri64v(std::uint32_t buffer, GLenum pname, std::int64_t * params) noexcept { return mygl::get_static_dispatch().glGetNamedBufferParameteri64v(buffer, pname, params); }
void glGetNamedBufferParameteriv(std::uint32_t buffer, GLenum pname, std::int32_t * params) noexcept { return mygl::get_static_dispatch().glGetNamedBufferParameteriv(buffer, pname, params); }
void glGetNamedBufferParameterui64vNV(std::uint32_t buffer, GLenum pname, std::uint64_t * params) noexcept { return mygl::get_static_dispatch().glGetNamedBufferParameterui64vNV(buffer, pname, params); }
void glGetNamedBufferPointerv(std::uint32_t buffer, GLenum pname, void ** params) noexcept { return mygl::get_static_dispatch().glGetNamedBufferPointerv(buffer, pname, params); }
void glGetNamedBufferSubData(std::uint32_t buffer, std::intptr_t offset, std::int64_t size, void * data) noexcept { return mygl::get_static_dispatch().glGetNamedBufferSubData(buffer, offset, size, data); }
void glGetNamedFramebufferAttachmentParameteriv(std::uint32_t framebuffer, GLenum attachment, GLenum pname, std::int32_t * params) noexcept { return mygl::get_static_dispatch().glGetNamedFramebufferAttachmentParameteriv(framebuffer, attachment, pname, params); }
void glGetNamedFramebufferParameteriv(std::uint32_t framebuffer, GLenum pname, std::int32_t * param) noexcept { return mygl::get_static_dispatch().glGetNamedFramebufferParameteriv(framebuffer, pname, param); }
void glGetNamedRenderbufferParameteriv(std::uint32_t renderbuffer, GLenum pname, std::int32_t * params) noexcept { return mygl::get_static_dispatch().glGetNamedRenderbufferParameteriv(renderbuffer, pname, params); }
void glGetObjectLabel(GLenum identifier, std::uint32_t name, std::int32_t bufSize, std::int32_t * length, char * label) noexcept { return mygl::get_static_dispatch().glGetObjectLabel(identifier, name, bufSize, length, label); }
void glGetObjectLabelKHR(GLenum identifier, std::uint32_t name, std::int32_t bufSize, std::int32_t * length, char * label) noexcept { return mygl::get_static_dispatch().glGetObjectLabelKHR(identifier, name, bufSize, length, label); }
void glGetObjectPtrLabel(const void * ptr, std::int32_t bufSize, std::int32_t * length, char * label) noexcept { return mygl::get_static_dispatch().glGetObjectPtrLabel(ptr, bufSize, length, label); }
void glGetObjectPtrLabelKHR(const void * ptr, std::int32_t bufSize, std::int32_t * length, char * label) noexcept { return mygl::get_static_dispatch().glGetObjectPtrLabelKHR(ptr, bufSize, length, label); }
void glGetPathColorGenfvNV(GLenum color, GLenum pname, float * value) noexcept { return mygl::get_static_dispatch().glGetPathColorGenfvNV(color, pname, value); }
void glGetPathColorGenivNV(GLenum color, GLenum pname, std::int32_t * value) noexcept { return mygl::get_static_dispatch().glGetPathColorGenivNV(color, pname, value); }
void glGetPathCommandsNV(std::uint32_t path, std::uint8_t * commands) noexcept { return mygl::get_static_dispatch().glGetPathCommandsNV(path, commands); }
void glGetPathCoordsNV(std::uint32_t path, float * coords) noexcept { return mygl::get_static_dispatch().glGetPathCoordsNV(path, coords); }
void glGetPathDashArrayNV(std::uint32_t path, float * dashArray) noexcept { return mygl::get_static_dispatch().glGetPathDashArrayNV(path, dashArray); }
float glGetPathLengthNV(std::uint32_t path, std::int32_t startSegment, std::int32_t numSegments) noexcept { return mygl::get_static_dispatch().glGetPathLengthNV(path, startSegment, numSegments); }
void glGetPathMetricRangeNV(GLbitfield metricQueryMask, std::uint32_t firstPathName, std::int32_t numPaths, std::int32_t stride, float * metrics) noexcept { return mygl::get_static_dispatch().glGetPathMetricRangeNV(metricQueryMask, firstPathName, numPaths, stride, metrics); }
void glGetPathMetricsNV(GLbitfield metricQueryMask, std::int32_t numPaths, GLenum pathNameType, const void * paths, std::uint32_t pathBase, std::int32_t stride, float * metrics) noexcept { return mygl::get_static_dispatch().glGetPathMetricsNV(metricQueryMask, numPaths, pathNameType, paths, pathBase, stride, metrics); }
void glGetPathParameterfvNV(std::uint32_t path, GLenum pname, float * value) noexcept { return mygl::get_static_dispatch().glGetPathParameterfvNV(path, pname, value); }
void glGetPathParameterivNV(std::uint32_t path, GLenum pname, std::int32_t * value) noexcept { return mygl::get_static_dispatch().glGetPathParameterivNV(path, pname, value); }
void glGetPathSpacingNV(GLenum pathListMode, std::int32_t numPaths, GLenum pathNameType, const void * paths, std::uint32_t pathBase, float advanceScale, float kerningScale, GLenum transformType, float * returnedSpacing) noexcept { return mygl::get_static_dispatch().glGetPathSpacingNV(pathListMode, numPaths, pathNameType, paths, pathBase, advanceScale, kerningScale, transformType, returnedSpacing); }
void glGetPathTexGenfvNV(GLenum texCoordSet, GLenum pname, float * value) noexcept { return mygl::get_static_dispatch().glGetPathTexGenfvNV(texCoordSet, pname, value); }
void glGetPathTexGenivNV(GLenum texCoordSet, GLenum pname, std::int32_t * value) noexcept { return mygl::get_static_dispatch().glGetPathTexGenivNV(texCoordSet, pname, value); }
void glGetPointerv(GLenum pname, void ** params) noexcept { return mygl::get_static_dispatch().glGetPointerv(pname, params); }
void glGetPointervKHR(GLenum pname, void ** params) noexcept { return mygl::get_static_dispatch().glGetPointervKHR(pname, params); }
void glGetProgramBinary(std::uint32_t program, std::int32_t bufSize, std::int32_t * length, GLenum * binaryFormat, void * binary) noexcept { return mygl::get_static_dispatch().glGetProgramBinary(program, bufSize, length, binaryFormat, binary); }
void glGetProgramInfoLog(std::uint32_t program, std::int32_t bufSize, std::int32_t * length, char * infoLog) noexcept { return mygl::get_static_dispatch().glGetProgramInfoLog(program, bufSize, length, infoLog); }
void glGetProgramInterfaceiv(std::uint32_t program, GLenum programInterface, GLenum pname, std::int32_t * params) noexcept { return mygl::get_static_dispatch().glGetProgramInterfaceiv(program, programInterface, pname, params); }
void glGetProgramPipelineInfoLog(std::uint32_t pipeline, std::int32_t bufSize, std::int32_t * length, char * infoLog) noexcept { return mygl::get_static_dispatch().glGetProgramPipelineInfoLog(pipeline, bufSize, length, infoLog); }
void glGetProgramPipelineiv(std::uint32_t pipeline, GLenum pname, std::int32_t * params) noexcept { return mygl::get_static_dispatch().glGetProgramPipelineiv(pipeline, pname, params); }
std::uint32_t glGetProgramResourceIndex(std::uint32_t program, GLenum programInterface, const char * name) noexcept { return mygl::get_static_dispatch().glGetProgramResourceIndex(program, programInterface, name); }
std::int32_t glGetProgramResourceLocation(std::uint32_t program, GLenum programInterface, const char * name) noexcept { return mygl::get_static_dispatch().glGetProgramResourceLocation(program, programInterface, name); }
std::int32_t glGetProgramResourceLocationIndex(std::uint32_t program, GLenum programInterface, const char * name) noexcept { return mygl::get_static_dispatch().glGetProgramResourceLocationIndex(program, programInterface, name); }
void glGetProgramResourceName(std::uint32_t program, GLenum programInterface, std::uint32_t index, std::int32_t bufSize, std::int32_t * length, char * name) noexcept { return mygl::get_static_dispatch().glGetProgramResourceName(program, programInterface, index, bufSize, length, name); }
void glGetProgramResourcefvNV(std::uint32_t program, GLenum programInterface, std::uint32_t index, std::int32_t propCount, const GLenum * props, std::int32_t count, std::int32_t * length, float * params) noexcept { return mygl::get_static_dispatch().glGetProgramResourcefvNV(program, programInterface, index, propCount, props, count, length, params); }
void glGetProgramResourceiv(std::uint32_t program, GLenum programInterface, std::uint32_t index, std::int32_t propCount, const GLenum * props, std::int32_t count, std::int32_t * length, std::int32_t * params) noexcept { return mygl::get_static_dispatch().glGetProgramResourceiv(program, programInterface, index, propCount, props, count, length, params); }
void glGetProgramStageiv(std::uint32_t program, GLenum shadertype, GLenum pname, std::int32_t * values) noexcept { return mygl::get_static_dispatch().glGetProgramStageiv(program, shadertype, pname, values); }
void glGetProgramSubroutineParameteruivNV(GLenum target, std::uint32_t index, std::uint32_t * param) noexcept { return mygl::get_static_dispatch().glGetProgramSubroutineParameteruivNV(target, index, param); }
void glGetProgramiv(std::uint32_t program, GLenum pname, std::int32_t * params) noexcept { return mygl::get_static_dispatch().glGetProgramiv(program, pname, params); }
void glGetQueryBufferObjecti64v(std::uint32_t id, std::uint32_t buffer, GLenum pname, std::intptr_t offset) noexcept { return mygl::get_static_dispatch().glGetQueryBufferObjecti64v(id, buffer, pname, offset); }
void glGetQueryBufferObjectiv(std::uint32_t id, std::uint32_t buffer, GLenum pname, std::intptr_t offset) noexcept { return mygl::get_static_dispatch().glGetQueryBufferObjectiv(id, buffer, pname, offset); }
void glGetQueryBufferObjectui64v(std::uint32_t id, std::uint32_t buffer, GLenum pname, std::intptr_t offset) noexcept { return mygl::get_static_dispatch().glGetQueryBufferObjectui64v(id, buffer, pname, offset); }
void glGetQueryBufferObjectuiv(std::uint32_t id, std::uint32_t buffer, GLenum pname, std::intptr_t offset) noexcept { return mygl::get_static_dispatch().glGetQueryBufferObjectuiv(id, buffer, pname, offset); }
void glGetQueryIndexediv(GLenum target, std::uint32_t index, GLenum pname, std::int32_t * params) noexcept { return mygl::get_static_dispatch().glGetQueryIndexediv(target, index, pname, params); }
void glGetQueryObjecti64v(std::uint32_t id, GLenum pname, std::int64_t * params) noexcept { return mygl::get_static_dispatch().glGetQueryObjecti64v(id, pname, params); }
void glGetQueryObjectiv(std::uint32_t id, GLenum pname, std::int32_t * params) noexcept { return mygl::get_static_dispatch().glGetQueryObjectiv(id, pname, params); }
void glGetQueryObjectui64v(std::uint32_t id, GLenum pname, std::uint64_t * params) noexcept { return mygl::get_static_dispatch().glGetQueryObjectui64v(id, pname, params); }
void glGetQueryObjectuiv(std::uint32_t id, GLenum pname, std::uint32_t * params) noexcept { return mygl::get_static_dispatch().glGetQueryObjectuiv(id, pname, params); }
void glGetQueryiv(GLenum target, GLenum pname, std::int32_t * params) noexcept { return mygl::get_static_dispatch().glGetQueryiv(target, pname, params); }
void glGetRenderbufferParameteriv(GLenum target, GLenum pname, std::int32_t * params) noexcept { return mygl::get_static_dispatch().glGetRenderbufferParameteriv(target, pname, params); }
void glGetSamplerParameterIiv(std::uint32_t sampler, GLenum pname, std::int32_t * params) noexcept { return mygl::get_static_dispatch().glGetSamplerParameterIiv(sampler, pname, params); }
void glGetSamplerParameterIuiv(std::uint32_t sampler, GLenum pname, std::uint32_t * params) noexcept { return mygl::get_static_dispatch().glGetSamplerParameterIuiv(sampler, pname, params); }
void glGetSamplerParameterfv(std::uint32_t sampler, GLenum pname, float * params) noexcept { return mygl::get_static_dispatch().glGetSamplerParameterfv(sampler, pname, params); }
void glGetSamplerParameteriv(std::uint32_t sampler, GLenum pname, std::int32_t * params) noexcept { return mygl::get_static_dispatch().glGetSamplerParameteriv(sampler, pname, params); }
void glGetSemaphoreParameterui64vEXT(std::uint32_t semaphore, GLenum pname, std::uint64_t * params) noexcept { return mygl::get_static_dispatch().glGetSemaphoreParameterui64vEXT(semaphore, pname, params); }
void glGetShaderInfoLog(std::uint32_t shader, std::int32_t bufSize, std::int32_t * length, char * infoLog) noexcept { return mygl::get_static_dispatch().glGetShaderInfoLog(shader, bufSize, length, infoLog); }
void glGetShaderPrecisionFormat(GLenum shadertype, GLenum precisiontype, std::int32_t * range, std::int32_t * precision) noexcept { return mygl::get_static_dispatch().glGetShaderPrecisionFormat(shadertype, precisiontype, range, precision); }
void glGetShaderSource(std::uint32_t shader, std::int32_t bufSize, std::int32_t * length, char * source) noexcept { return mygl::get_static_dispatch().glGetShaderSource(shader, bufSize, length, source); }
void glGetShaderiv(std::uint32_t shader, GLenum pname, std::int32_t * params) noexcept { return mygl::get_static_dispatch().glGetShaderiv(shader, pname, params); }
std::uint16_t glGetStageIndexNV(GLenum shadertype) noexcept { return mygl::get_static_dispatch().glGetStageIndexNV(shadertype); }
const std::uint8_t  *glGetString(GLenum name) noexcept { return mygl::get_static_dispatch().glGetString(name); }
const std::uint8_t  *glGetStringi(GLenum name, std::uint32_t index) noexcept { return mygl::get_static_dispatch().glGetStringi(name, index); }
std::uint32_t glGetSubroutineIndex(std::uint32_t program, GLenum shadertype, const char * name) noexcept { return mygl::get_static_dispatch().glGetSubroutineIndex(program, shadertype, name); }
std::int32_t glGetSubroutineUniformLocation(std::uint32_t program, GLenum shadertype, const char * name) noexcept { return mygl::get_static_dispatch().glGetSubroutineUniformLocation(program, shadertype, name); }
void glGetSynciv(struct __GLsync * sync, GLenum pname, std::int32_t count, std::int32_t * length, std::int32_t * values) noexcept { return mygl::get_static_dispatch().glGetSynciv(sync, pname, count, length, values); }
void glGetTexImage(GLenum target, std::int32_t level, GLenum format, GLenum type, void * pixels) noexcept { return mygl::get_static_dispatch().glGetTexImage(target, level, format, type, pixels); }
void glGetTexLevelParameterfv(GLenum target, std::int32_t level, GLenum pname, float * params) noexcept { return mygl::get_static_dispatch().glGetTexLevelParameterfv(target, level, pname, params); }
void glGetTexLevelParameteriv(GLenum target, std::int32_t level, GLenum pname, std::int32_t * params) noexcept { return mygl::get_static_dispatch().glGetTexLevelParameteriv(target, level, pname, params); }
void glGetTexParameterIiv(GLenum target, GLenum pname, std::int32_t * params) noexcept { return mygl::get_static_dispatch().glGetTexParameterIiv(target, pname, params); }
void glGetTexParameterIuiv(GLenum target, GLenum pname, std::uint32_t * params) noexcept { return mygl::get_static_dispatch().glGetTexParameterIuiv(target, pname, params); }
void glGetTexParameterfv(GLenum target, GLenum pname, float * params) noexcept { return mygl::get_static_dispatch().glGetTexParameterfv(target, pname, params); }
void glGetTexParameteriv(GLenum target, GLenum pname, std::int32_t * params) noexcept { return mygl::get_static_dispatch().glGetTexParameteriv(target, pname, params); }
std::uint64_t glGetTextureHandleARB(std::uint32_t texture) noexcept { return mygl::get_static_dispatch().glGetTextureHandleARB(texture); }
void glGetTextureImage(std::uint32_t texture, std::int32_t level, GLenum format, GLenum type, std::int32_t bufSize, void * pixels) noexcept { return mygl::get_static_dispatch().glGetTextureImage(texture, level, format, type, bufSize, pixels); }
void glGetTextureLevelParameterfv(std::uint32_t texture, std::int32_t level, GLenum pname, float * params) noexcept { return mygl::get_static_dispatch().glGetTextureLevelParameterfv(texture, level, pname, params); }
void glGetTextureLevelParameteriv(std::uint32_t texture, std::int32_t level, GLenum pname, std::int32_t * params) noexcept { return mygl::get_static_dispatch().glGetTextureLevelParameteriv(texture, level, pname, params); }
void glGetTextureParameterIiv(std::uint32_t texture, GLenum pname, std::int32_t * params) noexcept { return mygl::get_static_dispatch().glGetTextureParameterIiv(texture, pname, params); }
void glGetTextureParameterIuiv(std::uint32_t texture, GLenum pname, std::uint32_t * params) noexcept { return mygl::get_static_dispatch().glGetTextureParameterIuiv(texture, pname, params); }
void glGetTextureParameterfv(std::uint32_t texture, GLenum pname, float * params) noexcept { return mygl::get_static_dispatch().glGetTextureParameterfv(texture, pname, params); }
void glGetTextureParameteriv(std::uint32_t texture, GLenum pname, std::int32_t * params) noexcept { return mygl::get_static_dispatch().glGetTextureParameteriv(texture, pname, params); }
std::uint64_t glGetTextureSamplerHandleARB(std::uint32_t texture, std::uint32_t sampler) noexcept { return mygl::get_static_dispatch().glGetTextureSamplerHandleARB(texture, sampler); }
void glGetTextureSubImage(std::uint32_t texture, std::int32_t level, std::int32_t xoffset, std::int32_t yoffset, std::int32_t zoffset, std::int32_t width, std::int32_t height, std::int32_t depth, GLenum format, GLenum type, std::int32_t bufSize, void * pixels) noexcept { return mygl::get_static_dispatch().glGetTextureSubImage(texture, level, xoffset, yoffset, zoffset, width, height, depth, format, type, bufSize, pixels); }
void glGetTransformFeedbackVarying(std::uint32_t program, std::uint32_t index, std::int32_t bufSize, std::int32_t * length, std::int32_t * size, GLenum * type, char * name) noexcept { return mygl::get_static_dispatch().glGetTransformFeedbackVarying(program, index, bufSize, length, size, type, name); }
void glGetTransformFeedbacki64_v(std::uint32_t xfb, GLenum pname, std::uint32_t index, std::int64_t * param) noexcept { return mygl::get_static_dispatch().glGetTransformFeedbacki64_v(xfb, pname, index, param); }
void glGetTransformFeedbacki_v(std::uint32_t xfb, GLenum pname, std::uint32_t index, std::int32_t * param) noexcept { return mygl::get_static_dispatch().glGetTransformFeedbacki_v(xfb, pname, index, param); }
void glGetTransformFeedbackiv(std::uint32_t xfb, GLenum pname, std::int32_t * param) noexcept { return mygl::get_static_dispatch().glGetTransformFeedbackiv(xfb, pname, param); }
std::uint32_t glGetUniformBlockIndex(std::uint32_t program, const char * uniformBlockName) noexcept { return mygl::get_static_dispatch().glGetUniformBlockIndex(program, uniformBlockName); }
void glGetUniformIndices(std::uint32_t program, std::int32_t uniformCount, const char *const* uniformNames, std::uint32_t * uniformIndices) noexcept { return mygl::get_static_dispatch().glGetUniformIndices(program, uniformCount, uniformNames, uniformIndices); }
std::int32_t glGetUniformLocation(std::uint32_t program, const char * name) noexcept { return mygl::get_static_dispatch().glGetUniformLocation(program, name); }
void glGetUniformSubroutineuiv(GLenum shadertype, std::int32_t location, std::uint32_t * params) noexcept { return mygl::get_static_dispatch().glGetUniformSubroutineuiv(shadertype, location, params); }
void glGetUniformdv(std::uint32_t program, std::int32_t location, double * params) noexcept { return mygl::get_static_dispatch().glGetUniformdv(program, location, params); }
void glGetUniformfv(std::uint32_t program, std::int32_t location, float * params) noexcept { return mygl::get_static_dispatch().glGetUniformfv(program, location, params); }
void glGetUniformi64vARB(std::uint32_t program, std::int32_t location, std::int64_t * params) noexcept { return mygl::get_static_dispatch().glGetUniformi64vARB(program, location, params); }
void glGetUniformi64vNV(std::uint32_t program, std::int32_t location, std::int64_t * params) noexcept { return mygl::get_static_dispatch().glGetUniformi64vNV(program, location, params); }
void glGetUniformiv(std::uint32_t program, std::int32_t location, std::int32_t * params) noexcept { return mygl::get_static_dispatch().glGetUniformiv(program, location, params); }
void glGetUniformui64vARB(std::uint32_t program, std::int32_t location, std::uint64_t * params) noexcept { return mygl::get_static_dispatch().glGetUniformui64vARB(program, location, params); }
void glGetUniformui64vNV(std::uint32_t program, std::int32_t location, std::uint64_t * params) noexcept { return mygl::get_static_dispatch().glGetUniformui64vNV(program, location, params); }
void glGetUniformuiv(std::uint32_t program, std::int32_t location, std::uint32_t * params) noexcept { return mygl::get_static_dispatch().glGetUniformuiv(program, location, params); }
void glGetUnsignedBytevEXT(GLenum pname, std::uint8_t * data) noexcept { return mygl::get_static_dispatch().glGetUnsignedBytevEXT(pname, data); }
void glGetUnsignedBytei_vEXT(GLenum target, std::uint32_t index, std::uint8_t * data) noexcept { return mygl::get_static_dispatch().glGetUnsignedBytei_vEXT(target, index, data); }
void glGetVertexArrayIndexed64iv(std::uint32_t vaobj, std::uint32_t index, GLenum pname, std::int64_t * param) noexcept { return mygl::get_static_dispatch().glGetVertexArrayIndexed64iv(vaobj, index, pname, param); }
void glGetVertexArrayIndexediv(std::uint32_t vaobj, std::uint32_t index, GLenum pname, std::int32_t * param) noexcept { return mygl::get_static_dispatch().glGetVertexArrayIndexediv(vaobj, index, pname, param); }
void glGetVertexArrayiv(std::uint32_t vaobj, GLenum pname, std::int32_t * param) noexcept { return mygl::get_static_dispatch().glGetVertexArrayiv(vaobj, pname, param); }
void glGetVertexAttribIiv(std::uint32_t index, GLenum pname, std::int32_t * params) noexcept { return mygl::get_static_dispatch().glGetVertexAttribIiv(index, pname, params); }
void glGetVertexAttribIuiv(std::uint32_t index, GLenum pname, std::uint32_t * params) noexcept { return mygl::get_static_dispatch().glGetVertexAttribIuiv(index, pname, params); }
void glGetVertexAttribLdv(std::uint32_t index, GLenum pname, double * params) noexcept { return mygl::get_static_dispatch().glGetVertexAttribLdv(index, pname, params); }
void glGetVertexAttribLui64vARB(std::uint32_t index, GLenum pname, std::uint64_t * params) noexcept { return mygl::get_static_dispatch().glGetVertexAttribLui64vARB(index, pname, params); }
void glGetVertexAttribPointerv(std::uint32_t index, GLenum pname, void ** pointer) noexcept { return mygl::get_static_dispatch().glGetVertexAttribPointerv(index, pname, pointer); }
void glGetVertexAttribdv(std::uint32_t index, GLenum pname, double * params) noexcept { return mygl::get_static_dispatch().glGetVertexAttribdv(index, pname, params); }
void glGetVertexAttribfv(std::uint32_t index, GLenum pname, float * params) noexcept { return mygl::get_static_dispatch().glGetVertexAttribfv(index, pname, params); }
void glGetVertexAttribiv(std::uint32_t index, GLenum pname, std::int32_t * params) noexcept { return mygl::get_static_dispatch().glGetVertexAttribiv(index, pname, params); }
void glGetnCompressedTexImage(GLenum target, std::int32_t lod, std::int32_t bufSize, void * pixels) noexcept { return mygl::get_static_dispatch().glGetnCompressedTexImage(target, lod, bufSize, pixels); }
void glGetnTexImage(GLenum target, std::int32_t level, GLenum format, GLenum type, std::int32_t bufSize, void * pixels) noexcept { return mygl::get_static_dispatch().glGetnTexImage(target, level, format, type, bufSize, pixels); }
void glGetnUniformdv(std::uint32_t program, std::int32_t location, std::int32_t bufSize, double * params) noexcept { return mygl::get_static_dispatch().glGetnUniformdv(program, location, bufSize, params); }
void glGetnUniformfv(std::uint32_t program, std::int32_t location, std::int32_t bufSize, float * params) noexcept { return mygl::get_static_dispatch().glGetnUniformfv(program, location, bufSize, params); }
void glGetnUniformi64vARB(std::uint32_t program, std::int32_t location, std::int32_t bufSize, std::int64_t * params) noexcept { return mygl::get_static_dispatch().glGetnUniformi64vARB(program, location, bufSize, params); }
void glGetnUniformiv(std::uint32_t program, std::int32_t location, std::int32_t bufSize, std::int32_t * params) noexcept { return mygl::get_static_dispatch().glGetnUniformiv(program, location, bufSize, params); }
void glGetnUniformui64vARB(std::uint32_t program, std::int32_t location, std::int32_t bufSize, std::uint64_t * params) noexcept { return mygl::get_static_dispatch().glGetnUniformui64vARB(program, location, bufSize, params); }
void glGetnUniformuiv(std::uint32_t program, std::int32_t location, std::int32_t bufSize, std::uint32_t * params) noexcept { return mygl::get_static_dispatch().glGetnUniformuiv(program, location, bufSize, params); }
void glHint(GLenum target, GLenum mode) noexcept { return mygl::get_static_dispatch().glHint(target, mode); }
void glImportMemoryFdEXT(std::uint32_t memory, std::uint64_t size, GLenum handleType, std::int32_t fd) noexcept { return mygl::get_static_dispatch().glImportMemoryFdEXT(memory, size, handleType, fd); }
void glImportMemoryWin32HandleEXT(std::uint32_t memory, std::uint64_t size, GLenum handleType, void * handle) noexcept { return mygl::get_static_dispatch().glImportMemoryWin32HandleEXT(memory, size, handleType, handle); }
void glImportMemoryWin32NameEXT(std::uint32_t memory, std::uint64_t size, GLenum handleType, const void * name) noexcept { return mygl::get_static_dispatch().glImportMemoryWin32NameEXT(memory, size, handleType, name); }
void glImportSemaphoreFdEXT(std::uint32_t semaphore, GLenum handleType, std::int32_t fd) noexcept { return mygl::get_static_dispatch().glImportSemaphoreFdEXT(semaphore, handleType, fd); }
void glImportSemaphoreWin32HandleEXT(std::uint32_t semaphore, GLenum handleType, void * handle) noexcept { return mygl::get_static_dispatch().glImportSemaphoreWin32HandleEXT(semaphore, handleType, handle); }
void glImportSemaphoreWin32NameEXT(std::uint32_t semaphore, GLenum handleType, const void * name) noexcept { return mygl::get_static_dispatch().glImportSemaphoreWin32NameEXT(semaphore, handleType, name); }
void glIndexFormatNV(GLenum type, std::int32_t stride) noexcept { return mygl::get_static_dispatch().glIndexFormatNV(type, stride); }
void glInterpolatePathsNV(std::uint32_t resultPath, std::uint32_t pathA, std::uint32_t pathB, float weight) noexcept { return mygl::get_static_dispatch().glInterpolatePathsNV(resultPath, pathA, pathB, weight); }
void glInvalidateBufferData(std::uint32_t buffer) noexcept { return mygl::get_static_dispatch().glInvalidateBufferData(buffer); }
void glInvalidateBufferSubData(std::uint32_t buffer, std::intptr_t offset, std::int64_t length) noexcept { return mygl::get_static_dispatch().glInvalidateBufferSubData(buffer, offset, length); }
void glInvalidateFramebuffer(GLenum target, std::int32_t numAttachments, const GLenum * attachments) noexcept { return mygl::get_static_dispatch().glInvalidateFramebuffer(target, numAttachments, attachments); }
void glInvalidateNamedFramebufferData(std::uint32_t framebuffer, std::int32_t numAttachments, const GLenum * attachments) noexcept { return mygl::get_static_dispatch().glInvalidateNamedFramebufferData(framebuffer, numAttachments, attachments); }
void glInvalidateNamedFramebufferSubData(std::uint32_t framebuffer, std::int32_t numAttachments, const GLenum * attachments, std::int32_t x, std::int32_t y, std::int32_t width, std::int32_t height) noexcept { return mygl::get_static_dispatch().glInvalidateNamedFramebufferSubData(framebuffer, numAttachments, attachments, x, y, width, height); }
void glInvalidateSubFramebuffer(GLenum target, std::int32_t numAttachments, const GLenum * attachments, std::int32_t x, std::int32_t y, std::int32_t width, std::int32_t height) noexcept { return mygl::get_static_dispatch().glInvalidateSubFramebuffer(target, numAttachments, attachments, x, y, width, height); }
void glInvalidateTexImage(std::uint32_t texture, std::int32_t level) noexcept { return mygl::get_static_dispatch().glInvalidateTexImage(texture, level); }
void glInvalidateTexSubImage(std::uint32_t texture, std::int32_t level, std::int32_t xoffset, std::int32_t yoffset, std::int32_t zoffset, std::int32_t width, std::int32_t height, std::int32_t depth) noexcept { return mygl::get_static_dispatch().glInvalidateTexSubImage(texture, level, xoffset, yoffset, zoffset, width, height, depth); }
bool glIsBuffer(std::uint32_t buffer) noexcept { return mygl::get_static_dispatch().glIsBuffer(buffer); }
bool glIsBufferResidentNV(GLenum target) noexcept { return mygl::get_static_dispatch().glIsBufferResidentNV(target); }
bool glIsCommandListNV(std::uint32_t list) noexcept { return mygl::get_static_dispatch().glIsCommandListNV(list); }
bool glIsEnabled(GLenum cap) noexcept { return mygl::get_static_dispatch().glIsEnabled(cap); }
bool glIsEnabledi(GLenum target, std::uint32_t index) noexcept { return mygl::get_static_dispatch().glIsEnabledi(target, index); }
bool glIsFramebuffer(std::uint32_t framebuffer) noexcept { return mygl::get_static_dispatch().glIsFramebuffer(framebuffer); }
bool glIsImageHandleResidentARB(std::uint64_t handle) noexcept { return mygl::get_static_dispatch().glIsImageHandleResidentARB(handle); }
bool glIsMemoryObjectEXT(std::uint32_t memoryObject) noexcept { return mygl::get_static_dispatch().glIsMemoryObjectEXT(memoryObject); }
bool glIsNamedBufferResidentNV(std::uint32_t buffer) noexcept { return mygl::get_static_dispatch().glIsNamedBufferResidentNV(buffer); }
bool glIsPathNV(std::uint32_t path) noexcept { return mygl::get_static_dispatch().glIsPathNV(path); }
bool glIsPointInFillPathNV(std::uint32_t path, std::uint32_t mask, float x, float y) noexcept { return mygl::get_static_dispatch().glIsPointInFillPathNV(path, mask, x, y); }
bool glIsPointInStrokePathNV(std::uint32_t path, float x, float y) noexcept { return mygl::get_static_dispatch().glIsPointInStrokePathNV(path, x, y); }
bool glIsProgram(std::uint32_t program) noexcept { return mygl::get_static_dispatch().glIsProgram(program); }
bool glIsProgramPipeline(std::uint32_t pipeline) noexcept { return mygl::get_static_dispatch().glIsProgramPipeline(pipeline); }
bool glIsQuery(std::uint32_t id) noexcept { return mygl::get_static_dispatch().glIsQuery(id); }
bool glIsRenderbuffer(std::uint32_t renderbuffer) noexcept { return mygl::get_static_dispatch().glIsRenderbuffer(renderbuffer); }
bool glIsSemaphoreEXT(std::uint32_t semaphore) noexcept { return mygl::get_static_dispatch().glIsSemaphoreEXT(semaphore); }
bool glIsSampler(std::uint32_t sampler) noexcept { return mygl::get_static_dispatch().glIsSampler(sampler); }
bool glIsShader(std::uint32_t shader) noexcept { return mygl::get_static_dispatch().glIsShader(shader); }
bool glIsStateNV(std::uint32_t state) noexcept { return mygl::get_static_dispatch().glIsStateNV(state); }
bool glIsSync(struct __GLsync * sync) noexcept { return mygl::get_static_dispatch().glIsSync(sync); }
bool glIsTexture(std::uint32_t texture) noexcept { return mygl::get_static_dispatch().glIsTexture(texture); }
bool glIsTextureHandleResidentARB(std::uint64_t handle) noexcept { return mygl::get_static_dispatch().glIsTextureHandleResidentARB(handle); }
bool glIsTransformFeedback(std::uint32_t id) noexcept { return mygl::get_static_dispatch().glIsTransformFeedback(id); }
bool glIsVertexArray(std::uint32_t array) noexcept { return mygl::get_static_dispatch().glIsVertexArray(array); }
void glLineWidth(float width) noexcept { return mygl::get_static_dispatch().glLineWidth(width); }
void glLinkProgram(std::uint32_t program) noexcept { return mygl::get_static_dispatch().glLinkProgram(program); }
void glListDrawCommandsStatesClientNV(std::uint32_t list, std::uint32_t segment, const void ** indirects, const std::int32_t * sizes, const std::uint32_t * states, const std::uint32_t * fbos, std::uint32_t count) noexcept { return mygl::get_static_dispatch().glListDrawCommandsStatesClientNV(list, segment, indirects, sizes, states, fbos, count); }
void glLogicOp(GLenum opcode) noexcept { return mygl::get_static_dispatch().glLogicOp(opcode); }
void glMakeBufferNonResidentNV(GLenum target) noexcept { return mygl::get_static_dispatch().glMakeBufferNonResidentNV(target); }
void glMakeBufferResidentNV(GLenum target, GLenum access) noexcept { return mygl::get_static_dispatch().glMakeBufferResidentNV(target, access); }
void glMakeImageHandleNonResidentARB(std::uint64_t handle) noexcept { return mygl::get_static_dispatch().glMakeImageHandleNonResidentARB(handle); }
void glMakeImageHandleResidentARB(std::uint64_t handle, GLenum access) noexcept { return mygl::get_static_dispatch().glMakeImageHandleResidentARB(handle, access); }
void glMakeNamedBufferNonResidentNV(std::uint32_t buffer) noexcept { return mygl::get_static_dispatch().glMakeNamedBufferNonResidentNV(buffer); }
void glMakeNamedBufferResidentNV(std::uint32_t buffer, GLenum access) noexcept { return mygl::get_static_dispatch().glMakeNamedBufferResidentNV(buffer, access); }
void glMakeTextureHandleNonResidentARB(std::uint64_t handle) noexcept { return mygl::get_static_dispatch().glMakeTextureHandleNonResidentARB(handle); }
void glMakeTextureHandleResidentARB(std::uint64_t handle) noexcept { return mygl::get_static_dispatch().glMakeTextureHandleResidentARB(handle); }
void *glMapBuffer(GLenum target, GLenum access) noexcept { return mygl::get_static_dispatch().glMapBuffer(target, access); }
void *glMapBufferRange(GLenum target, std::intptr_t offset, std::int64_t length, GLbitfield access) noexcept { return mygl::get_static_dispatch().glMapBufferRange(target, offset, length, access); }
void *glMapNamedBuffer(std::uint32_t buffer, GLenum access) noexcept { return mygl::get_static_dispatch().glMapNamedBuffer(buffer, access); }
void *glMapNamedBufferRange(std::uint32_t buffer, std::intptr_t offset, std::int64_t length, GLbitfield access) noexcept { return mygl::get_static_dispatch().glMapNamedBufferRange(buffer, offset, length, access); }
void glMatrixFrustumEXT(GLenum mode, double left, double right, double bottom, double top, double zNear, double zFar) noexcept { return mygl::get_static_dispatch().glMatrixFrustumEXT(mode, left, right, bottom, top, zNear, zFar); }
void glMatrixLoad3x2fNV(GLenum matrixMode, const float * m) noexcept { return mygl::get_static_dispatch().glMatrixLoad3x2fNV(matrixMode, m); }
void glMatrixLoad3x3fNV(GLenum matrixMode, const float * m) noexcept { return mygl::get_static_dispatch().glMatrixLoad3x3fNV(matrixMode, m); }
void glMatrixLoadIdentityEXT(GLenum mode) noexcept { return mygl::get_static_dispatch().glMatrixLoadIdentityEXT(mode); }
void glMatrixLoadTranspose3x3fNV(GLenum matrixMode, const float * m) noexcept { return mygl::get_static_dispatch().glMatrixLoadTranspose3x3fNV(matrixMode, m); }
void glMatrixLoadTransposedEXT(GLenum mode, const double * m) noexcept { return mygl::get_static_dispatch().glMatrixLoadTransposedEXT(mode, m); }
void glMatrixLoadTransposefEXT(GLenum mode, const float * m) noexcept { return mygl::get_static_dispatch().glMatrixLoadTransposefEXT(mode, m); }
void glMatrixLoaddEXT(GLenum mode, const double * m) noexcept { return mygl::get_static_dispatch().glMatrixLoaddEXT(mode, m); }
void glMatrixLoadfEXT(GLenum mode, const float * m) noexcept { return mygl::get_static_dispatch().glMatrixLoadfEXT(mode, m); }
void glMatrixMult3x2fNV(GLenum matrixMode, const float * m) noexcept { return mygl::get_static_dispatch().glMatrixMult3x2fNV(matrixMode, m); }
void glMatrixMult3x3fNV(GLenum matrixMode, const float * m) noexcept { return mygl::get_static_dispatch().glMatrixMult3x3fNV(matrixMode, m); }
void glMatrixMultTranspose3x3fNV(GLenum matrixMode, const float * m) noexcept { return mygl::get_static_dispatch().glMatrixMultTranspose3x3fNV(matrixMode, m); }
void glMatrixMultTransposedEXT(GLenum mode, const double * m) noexcept { return mygl::get_static_dispatch().glMatrixMultTransposedEXT(mode, m); }
void glMatrixMultTransposefEXT(GLenum mode, const float * m) noexcept { return mygl::get_static_dispatch().glMatrixMultTransposefEXT(mode, m); }
void glMatrixMultdEXT(GLenum mode, const double * m) noexcept { return mygl::get_static_dispatch().glMatrixMultdEXT(mode, m); }
void glMatrixMultfEXT(GLenum mode, const float * m) noexcept { return mygl::get_static_dispatch().glMatrixMultfEXT(mode, m); }
void glMatrixOrthoEXT(GLenum mode, double left, double right, double bottom, double top, double zNear, double zFar) noexcept { return mygl::get_static_dispatch().glMatrixOrthoEXT(mode, left, right, bottom, top, zNear, zFar); }
void glMatrixPopEXT(GLenum mode) noexcept { return mygl::get_static_dispatch().glMatrixPopEXT(mode); }
void glMatrixPushEXT(GLenum mode) noexcept { return mygl::get_static_dispatch().glMatrixPushEXT(mode); }
void glMatrixRotatedEXT(GLenum mode, double angle, double x, double y, double z) noexcept { return mygl::get_static_dispatch().glMatrixRotatedEXT(mode, angle, x, y, z); }
void glMatrixRotatefEXT(GLenum mode, float angle, float x, float y, float z) noexcept { return mygl::get_static_dispatch().glMatrixRotatefEXT(mode, angle, x, y, z); }
void glMatrixScaledEXT(GLenum mode, double x, double y, double z) noexcept { return mygl::get_static_dispatch().glMatrixScaledEXT(mode, x, y, z); }
void glMatrixScalefEXT(GLenum mode, float x, float y, float z) noexcept { return mygl::get_static_dispatch().glMatrixScalefEXT(mode, x, y, z); }
void glMatrixTranslatedEXT(GLenum mode, double x, double y, double z) noexcept { return mygl::get_static_dispatch().glMatrixTranslatedEXT(mode, x, y, z); }
void glMatrixTranslatefEXT(GLenum mode, float x, float y, float z) noexcept { return mygl::get_static_dispatch().glMatrixTranslatefEXT(mode, x, y, z); }
void glMemoryBarrier(GLbitfield barriers) noexcept { return mygl::get_static_dispatch().glMemoryBarrier(barriers); }
void glMemoryBarrierByRegion(GLbitfield barriers) noexcept { return mygl::get_static_dispatch().glMemoryBarrierByRegion(barriers); }
void glMemoryObjectParameterivEXT(std::uint32_t memoryObject, GLenum pname, const std::int32_t * params) noexcept { return mygl::get_static_dispatch().glMemoryObjectParameterivEXT(memoryObject, pname, params); }
void glMinSampleShading(float value) noexcept { return mygl::get_static_dispatch().glMinSampleShading(value); }
void glMultiDrawArrays(GLenum mode, const std::int32_t * first, const std::int32_t * count, std::int32_t drawcount) noexcept { return mygl::get_static_dispatch().glMultiDrawArrays(mode, first, count, drawcount); }
void glMultiDrawArraysIndirect(GLenum mode, const void * indirect, std::int32_t drawcount, std::int32_t stride) noexcept { return mygl::get_static_dispatch().glMultiDrawArraysIndirect(mode, indirect, drawcount, stride); }
void glMultiDrawArraysIndirectBindlessCountNV(GLenum mode, const void * indirect, std::int32_t drawCount, std::int32_t maxDrawCount, std::int32_t stride, std::int32_t vertexBufferCount) noexcept { return mygl::get_static_dispatch().glMultiDrawArraysIndirectBindlessCountNV(mode, indirect, drawCount, maxDrawCount, stride, vertexBufferCount); }
void glMultiDrawArraysIndirectBindlessNV(GLenum mode, const void * indirect, std::int32_t drawCount, std::int32_t stride, std::int32_t vertexBufferCount) noexcept { return mygl::get_static_dispatch().glMultiDrawArraysIndirectBindlessNV(mode, indirect, drawCount, stride, vertexBufferCount); }
void glMultiDrawArraysIndirectCount(GLenum mode, const void * indirect, std::intptr_t drawcount, std::int32_t maxdrawcount, std::int32_t stride) noexcept { return mygl::get_static_dispatch().glMultiDrawArraysIndirectCount(mode, indirect, drawcount, maxdrawcount, stride); }
void glMultiDrawElements(GLenum mode, const std::int32_t * count, GLenum type, const void *const* indices, std::int32_t drawcount) noexcept { return mygl::get_static_dispatch().glMultiDrawElements(mode, count, type, indices, drawcount); }
void glMultiDrawElementsBaseVertex(GLenum mode, const std::int32_t * count, GLenum type, const void *const* indices, std::int32_t drawcount, const std::int32_t * basevertex) noexcept { return mygl::get_static_dispatch().glMultiDrawElementsBaseVertex(mode, count, type, indices, drawcount, basevertex); }
void glMultiDrawElementsIndirect(GLenum mode, GLenum type, const void * indirect, std::int32_t drawcount, std::int32_t stride) noexcept { return mygl::get_static_dispatch().glMultiDrawElementsIndirect(mode, type, indirect, drawcount, stride); }
void glMultiDrawElementsIndirectBindlessCountNV(GLenum mode, GLenum type, const void * indirect, std::int32_t drawCount, std::int32_t maxDrawCount, std::int32_t stride, std::int32_t vertexBufferCount) noexcept { return mygl::get_static_dispatch().glMultiDrawElementsIndirectBindlessCountNV(mode, type, indirect, drawCount, maxDrawCount, stride, vertexBufferCount); }
void glMultiDrawElementsIndirectBindlessNV(GLenum mode, GLenum type, const void * indirect, std::int32_t drawCount, std::int32_t stride, std::int32_t vertexBufferCount) noexcept { return mygl::get_static_dispatch().glMultiDrawElementsIndirectBindlessNV(mode, type, indirect, drawCount, stride, vertexBufferCount); }
void glMultiDrawElementsIndirectCount(GLenum mode, GLenum type, const void * indirect, std::intptr_t drawcount, std::int32_t maxdrawcount, std::int32_t stride) noexcept { return mygl::get_static_dispatch().glMultiDrawElementsIndirectCount(mode, type, indirect, drawcount, maxdrawcount, stride); }
void glMultiTexCoordP1ui(GLenum texture, GLenum type, std::uint32_t coords) noexcept { return mygl::get_static_dispatch().glMultiTexCoordP1ui(texture, type, coords); }
void glMultiTexCoordP1uiv(GLenum texture, GLenum type, const std::uint32_t * coords) noexcept { return mygl::get_static_dispatch().glMultiTexCoordP1uiv(texture, type, coords); }
void glMultiTexCoordP2ui(GLenum texture, GLenum type, std::uint32_t coords) noexcept { return mygl::get_static_dispatch().glMultiTexCoordP2ui(texture, type, coords); }
void glMultiTexCoordP2uiv(GLenum texture, GLenum type, const std::uint32_t * coords) noexcept { return mygl::get_static_dispatch().glMultiTexCoordP2uiv(texture, type, coords); }
void glMultiTexCoordP3ui(GLenum texture, GLenum type, std::uint32_t coords) noexcept { return mygl::get_static_dispatch().glMultiTexCoordP3ui(texture, type, coords); }
void glMultiTexCoordP3uiv(GLenum texture, GLenum type, const std::uint32_t * coords) noexcept { return mygl::get_static_dispatch().glMultiTexCoordP3uiv(texture, type, coords); }
void glMultiTexCoordP4ui(GLenum texture, GLenum type, std::uint32_t coords) noexcept { return mygl::get_static_dispatch().glMultiTexCoordP4ui(texture, type, coords); }
void glMultiTexCoordP4uiv(GLenum texture, GLenum type, const std::uint32_t * coords) noexcept { return mygl::get_static_dispatch().glMultiTexCoordP4uiv(texture, type, coords); }
void glNamedBufferData(std::uint32_t buffer, std::int64_t size, const void * data, GLenum usage) noexcept { return mygl::get_static_dispatch().glNamedBufferData(buffer, size, data, usage); }
void glNamedBufferStorage(std::uint32_t buffer, std::int64_t size, const void * data, GLbitfield flags) noexcept { return mygl::get_static_dispatch().glNamedBufferStorage(buffer, size, data, flags); }
void glNamedBufferStorageMemEXT(std::uint32_t buffer, std::int64_t size, std::uint32_t memory, std::uint64_t offset) noexcept { return mygl::get_static_dispatch().glNamedBufferStorageMemEXT(buffer, size, memory, offset); }
void glNamedBufferSubData(std::uint32_t buffer, std::intptr_t offset, std::int64_t size, const void * data) noexcept { return mygl::get_static_dispatch().glNamedBufferSubData(buffer, offset, size, data); }
void glNamedFramebufferDrawBuffer(std::uint32_t framebuffer, GLenum buf) noexcept { return mygl::get_static_dispatch().glNamedFramebufferDrawBuffer(framebuffer, buf); }
void glNamedFramebufferDrawBuffers(std::uint32_t framebuffer, std::int32_t n, const GLenum * bufs) noexcept { return mygl::get_static_dispatch().glNamedFramebufferDrawBuffers(framebuffer, n, bufs); }
void glNamedFramebufferParameteri(std::uint32_t framebuffer, GLenum pname, std::int32_t param) noexcept { return mygl::get_static_dispatch().glNamedFramebufferParameteri(framebuffer, pname, param); }
void glNamedFramebufferReadBuffer(std::uint32_t framebuffer, GLenum src) noexcept { return mygl::get_static_dispatch().glNamedFramebufferReadBuffer(framebuffer, src); }
void glNamedFramebufferRenderbuffer(std::uint32_t framebuffer, GLenum attachment, GLenum renderbuffertarget, std::uint32_t renderbuffer) noexcept { return mygl::get_static_dispatch().glNamedFramebufferRenderbuffer(framebuffer, attachment, renderbuffertarget, renderbuffer); }
void glNamedFramebufferTexture(std::uint32_t framebuffer, GLenum attachment, std::uint32_t texture, std::int32_t level) noexcept { return mygl::get_static_dispatch().glNamedFramebufferTexture(framebuffer, attachment, texture, level); }
void glNamedFramebufferTextureLayer(std::uint32_t framebuffer, GLenum attachment, std::uint32_t texture, std::int32_t level, std::int32_t layer) noexcept { return mygl::get_static_dispatch().glNamedFramebufferTextureLayer(framebuffer, attachment, texture, level, layer); }
void glNamedRenderbufferStorage(std::uint32_t renderbuffer, GLenum internalformat, std::int32_t width, std::int32_t height) noexcept { return mygl::get_static_dispatch().glNamedRenderbufferStorage(renderbuffer, internalformat, width, height); }
void glNamedRenderbufferStorageMultisample(std::uint32_t renderbuffer, std::int32_t samples, GLenum internalformat, std::int32_t width, std::int32_t height) noexcept { return mygl::get_static_dispatch().glNamedRenderbufferStorageMultisample(renderbuffer, samples, internalformat, width, height); }
void glNormalFormatNV(GLenum type, std::int32_t stride) noexcept { return mygl::get_static_dispatch().glNormalFormatNV(type, stride); }
void glNormalP3ui(GLenum type, std::uint32_t coords) noexcept { return mygl::get_static_dispatch().glNormalP3ui(type, coords); }
void glNormalP3uiv(GLenum type, const std::uint32_t * coords) noexcept { return mygl::get_static_dispatch().glNormalP3uiv(type, coords); }
void glObjectLabel(GLenum identifier, std::uint32_t name, std::int32_t length, const char * label) noexcept { return mygl::get_static_dispatch().glObjectLabel(identifier, name, length, label); }
void glObjectLabelKHR(GLenum identifier, std::uint32_t name, std::int32_t length, const char * label) noexcept { return mygl::get_static_dispatch().glObjectLabelKHR(identifier, name, length, label); }
void glObjectPtrLabel(const void * ptr, std::int32_t length, const char * label) noexcept { return mygl::get_static_dispatch().glObjectPtrLabel(ptr, length, label); }
void glObjectPtrLabelKHR(const void * ptr, std::int32_t length, const char * label) noexcept { return mygl::get_static_dispatch().glObjectPtrLabelKHR(ptr, length, label); }
void glPatchParameterfv(GLenum pname, const float * values) noexcept { return mygl::get_static_dispatch().glPatchParameterfv(pname, values); }
void glPatchParameteri(GLenum pname, std::int32_t value) noexcept { return mygl::get_static_dispatch().glPatchParameteri(pname, value); }
void glPathColorGenNV(GLenum color, GLenum genMode, GLenum colorFormat, const float * coeffs) noexcept { return mygl::get_static_dispatch().glPathColorGenNV(color, genMode, colorFormat, coeffs); }
void glPathCommandsNV(std::uint32_t path, std::int32_t numCommands, const std::uint8_t * commands, std::int32_t numCoords, GLenum coordType, const void * coords) noexcept { return mygl::get_static_dispatch().glPathCommandsNV(path, numCommands, commands, numCoords, coordType, coords); }
void glPathCoordsNV(std::uint32_t path, std::int32_t numCoords, GLenum coordType, const void * coords) noexcept { return mygl::get_static_dispatch().glPathCoordsNV(path, numCoords, coordType, coords); }
void glPathCoverDepthFuncNV(GLenum func) noexcept { return mygl::get_static_dispatch().glPathCoverDepthFuncNV(func); }
void glPathDashArrayNV(std::uint32_t path, std::int32_t dashCount, const float * dashArray) noexcept { return mygl::get_static_dispatch().glPathDashArrayNV(path, dashCount, dashArray); }
void glPathFogGenNV(GLenum genMode) noexcept { return mygl::get_static_dispatch().glPathFogGenNV(genMode); }
GLenum glPathGlyphIndexArrayNV(std::uint32_t firstPathName, GLenum fontTarget, const void * fontName, GLbitfield fontStyle, std::uint32_t firstGlyphIndex, std::int32_t numGlyphs, std::uint32_t pathParameterTemplate, float emScale) noexcept { return mygl::get_static_dispatch().glPathGlyphIndexArrayNV(firstPathName, fontTarget, fontName, fontStyle, firstGlyphIndex, numGlyphs, pathParameterTemplate, emScale); }
GLenum glPathGlyphIndexRangeNV(GLenum fontTarget, const void * fontName, GLbitfield fontStyle, std::uint32_t pathParameterTemplate, float emScale, std::uint32_t * baseAndCount) noexcept { return mygl::get_static_dispatch().glPathGlyphIndexRangeNV(fontTarget, fontName, fontStyle, pathParameterTemplate, emScale, baseAndCount); }
void glPathGlyphRangeNV(std::uint32_t firstPathName, GLenum fontTarget, const void * fontName, GLbitfield fontStyle, std::uint32_t firstGlyph, std::int32_t numGlyphs, GLenum handleMissingGlyphs, std::uint32_t pathParameterTemplate, float emScale) noexcept { return mygl::get_static_dispatch().glPathGlyphRangeNV(firstPathName, fontTarget, fontName, fontStyle, firstGlyph, numGlyphs, handleMissingGlyphs, pathParameterTemplate, emScale); }
void glPathGlyphsNV(std::uint32_t firstPathName, GLenum fontTarget, const void * fontName, GLbitfield fontStyle, std::int32_t numGlyphs, GLenum type, const void * charcodes, GLenum handleMissingGlyphs, std::uint32_t pathParameterTemplate, float emScale) noexcept { return mygl::get_static_dispatch().glPathGlyphsNV(firstPathName, fontTarget, fontName, fontStyle, numGlyphs, type, charcodes, handleMissingGlyphs, pathParameterTemplate, emScale); }
GLenum glPathMemoryGlyphIndexArrayNV(std::uint32_t firstPathName, GLenum fontTarget, std::int64_t fontSize, const void * fontData, std::int32_t faceIndex, std::uint32_t firstGlyphIndex, std::int32_t numGlyphs, std::uint32_t pathParameterTemplate, float emScale) noexcept { return mygl::get_static_dispatch().glPathMemoryGlyphIndexArrayNV(firstPathName, fontTarget, fontSize, fontData, faceIndex, firstGlyphIndex, numGlyphs, pathParameterTemplate, emScale); }
void glPathParameterfNV(std::uint32_t path, GLenum pname, float value) noexcept { return mygl::get_static_dispatch().glPathParameterfNV(path, pname, value); }
void glPathParameterfvNV(std::uint32_t path, GLenum pname, const float * value) noexcept { return mygl::get_static_dispatch().glPathParameterfvNV(path, pname, value); }
void glPathParameteriNV(std::uint32_t path, GLenum pname, std::int32_t value) noexcept { return mygl::get_static_dispatch().glPathParameteriNV(path, pname, value); }
void glPathParameterivNV(std::uint32_t path, GLenum pname, const std::int32_t * value) noexcept { return mygl::get_static_dispatch().glPathParameterivNV(path, pname, value); }
void glPathStencilDepthOffsetNV(float factor, float units) noexcept { return mygl::get_static_dispatch().glPathStencilDepthOffsetNV(factor, units); }
void glPathStencilFuncNV(GLenum func, std::int32_t ref, std::uint32_t mask) noexcept { return mygl::get_static_dispatch().glPathStencilFuncNV(func, ref, mask); }
void glPathStringNV(std::uint32_t path, GLenum format, std::int32_t length, const void * pathString) noexcept { return mygl::get_static_dispatch().glPathStringNV(path, format, length, pathString); }
void glPathSubCommandsNV(std::uint32_t path, std::int32_t commandStart, std::int32_t commandsToDelete, std::int32_t numCommands, const std::uint8_t * commands, std::int32_t numCoords, GLenum coordType, const void * coords) noexcept { return mygl::get_static_dispatch().glPathSubCommandsNV(path, commandStart, commandsToDelete, numCommands, commands, numCoords, coordType, coords); }
void glPathSubCoordsNV(std::uint32_t path, std::int32_t coordStart, std::int32_t numCoords, GLenum coordType, const void * coords) noexcept { return mygl::get_static_dispatch().glPathSubCoordsNV(path, coordStart, numCoords, coordType, coords); }
void glPathTexGenNV(GLenum texCoordSet, GLenum genMode, std::int32_t components, const float * coeffs) noexcept { return mygl::get_static_dispatch().glPathTexGenNV(texCoordSet, genMode, components, coeffs); }
void glPauseTransformFeedback() noexcept { return mygl::get_static_dispatch().glPauseTransformFeedback(); }
void glPixelStoref(GLenum pname, float param) noexcept { return mygl::get_static_dispatch().glPixelStoref(pname, param); }
void glPixelStorei(GLenum pname, std::int32_t param) noexcept { return mygl::get_static_dispatch().glPixelStorei(pname, param); }
bool glPointAlongPathNV(std::uint32_t path, std::int32_t startSegment, std::int32_t numSegments, float distance, float * x, float * y, float * tangentX, float * tangentY) noexcept { return mygl::get_static_dispatch().glPointAlongPathNV(path, startSegment, numSegments, distance, x, y, tangentX, tangentY); }
void glPointParameterf(GLenum pname, float param) noexcept { return mygl::get_static_dispatch().glPointParameterf(pname, param); }
void glPointParameterfv(GLenum pname, const float * params) noexcept { return mygl::get_static_dispatch().glPointParameterfv(pname, params); }
void glPointParameteri(GLenum pname, std::int32_t param) noexcept { return mygl::get_static_dispatch().glPointParameteri(pname, param); }
void glPointParameteriv(GLenum pname, const std::int32_t * params) noexcept { return mygl::get_static_dispatch().glPointParameteriv(pname, params); }
void glPointSize(float size) noexcept { return mygl::get_static_dispatch().glPointSize(size); }
void glPolygonMode(GLenum face, GLenum mode) noexcept { return mygl::get_static_dispatch().glPolygonMode(face, mode); }
void glPolygonOffset(float factor, float units) noexcept { return mygl::get_static_dispatch().glPolygonOffset(factor, units); }
void glPolygonOffsetClamp(float factor, float units, float clamp) noexcept { return mygl::get_static_dispatch().glPolygonOffsetClamp(factor, units, clamp); }
void glPolygonOffsetClampEXT(float factor, float units, float clamp) noexcept { return mygl::get_static_dispatch().glPolygonOffsetClampEXT(factor, units, clamp); }
void glPopDebugGroup() noexcept { return mygl::get_static_dispatch().glPopDebugGroup(); }
void glPopDebugGroupKHR() noexcept { return mygl::get_static_dispatch().glPopDebugGroupKHR(); }
void glPrimitiveRestartIndex(std::uint32_t index) noexcept { return mygl::get_static_dispatch().glPrimitiveRestartIndex(index); }
void glProgramBinary(std::uint32_t program, GLenum binaryFormat, const void * binary, std::int32_t length) noexcept { return mygl::get_static_dispatch().glProgramBinary(program, binaryFormat, binary, length); }
void glProgramParameteri(std::uint32_t program, GLenum pname, std::int32_t value) noexcept { return mygl::get_static_dispatch().glProgramParameteri(program, pname, value); }
void glProgramPathFragmentInputGenNV(std::uint32_t program, std::int32_t location, GLenum genMode, std::int32_t components, const float * coeffs) noexcept { return mygl::get_static_dispatch().glProgramPathFragmentInputGenNV(program, location, genMode, components, coeffs); }
void glProgramSubroutineParametersuivNV(GLenum target, std::int32_t count, const std::uint32_t * params) noexcept { return mygl::get_static_dispatch().glProgramSubroutineParametersuivNV(target, count, params); }
void glProgramUniform1d(std::uint32_t program, std::int32_t location, double v0) noexcept { return mygl::get_static_dispatch().glProgramUniform1d(program, location, v0); }
void glProgramUniform1dv(std::uint32_t program, std::int32_t location, std::int32_t count, const double * value) noexcept { return mygl::get_static_dispatch().glProgramUniform1dv(program, location, count, value); }
void glProgramUniform1f(std::uint32_t program, std::int32_t location, float v0) noexcept { return mygl::get_static_dispatch().glProgramUniform1f(program, location, v0); }
void glProgramUniform1fv(std::uint32_t program, std::int32_t location, std::int32_t count, const float * value) noexcept { return mygl::get_static_dispatch().glProgramUniform1fv(program, location, count, value); }
void glProgramUniform1i(std::uint32_t program, std::int32_t location, std::int32_t v0) noexcept { return mygl::get_static_dispatch().glProgramUniform1i(program, location, v0); }
void glProgramUniform1i64ARB(std::uint32_t program, std::int32_t location, std::int64_t x) noexcept { return mygl::get_static_dispatch().glProgramUniform1i64ARB(program, location, x); }
void glProgramUniform1i64NV(std::uint32_t program, std::int32_t location, std::int64_t x) noexcept { return mygl::get_static_dispatch().glProgramUniform1i64NV(program, location, x); }
void glProgramUniform1i64vARB(std::uint32_t program, std::int32_t location, std::int32_t count, const std::int64_t * value) noexcept { return mygl::get_static_dispatch().glProgramUniform1i64vARB(program, location, count, value); }
void glProgramUniform1i64vNV(std::uint32_t program, std::int32_t location, std::int32_t count, const std::int64_t * value) noexcept { return mygl::get_static_dispatch().glProgramUniform1i64vNV(program, location, count, value); }
void glProgramUniform1iv(std::uint32_t program, std::int32_t location, std::int32_t count, const std::int32_t * value) noexcept { return mygl::get_static_dispatch().glProgramUniform1iv(program, location, count, value); }
void glProgramUniform1ui(std::uint32_t program, std::int32_t location, std::uint32_t v0) noexcept { return mygl::get_static_dispatch().glProgramUniform1ui(program, location, v0); }
void glProgramUniform1ui64ARB(std::uint32_t program, std::int32_t location, std::uint64_t x) noexcept { return mygl::get_static_dispatch().glProgramUniform1ui64ARB(program, location, x); }
void glProgramUniform1ui64NV(std::uint32_t program, std::int32_t location, std::uint64_t x) noexcept { return mygl::get_static_dispatch().glProgramUniform1ui64NV(program, location, x); }
void glProgramUniform1ui64vARB(std::uint32_t program, std::int32_t location, std::int32_t count, const std::uint64_t * value) noexcept { return mygl::get_static_dispatch().glProgramUniform1ui64vARB(program, location, count, value); }
void glProgramUniform1ui64vNV(std::uint32_t program, std::int32_t location, std::int32_t count, const std::uint64_t * value) noexcept { return mygl::get_static_dispatch().glProgramUniform1ui64vNV(program, location, count, value); }
void glProgramUniform1uiv(std::uint32_t program, std::int32_t location, std::int32_t count, const std::uint32_t * value) noexcept { return mygl::get_static_dispatch().glProgramUniform1uiv(program, location, count, value); }
void glProgramUniform2d(std::uint32_t program, std::int32_t location, double v0, double v1) noexcept { return mygl::get_static_dispatch().glProgramUniform2d(program, location, v0, v1); }
void glProgramUniform2dv(std::uint32_t program, std::int32_t location, std::int32_t count, const double * value) noexcept { return mygl::get_static_dispatch().glProgramUniform2dv(program, location, count, value); }
void glProgramUniform2f(std::uint32_t program, std::int32_t location, float v0, float v1) noexcept { return mygl::get_static_dispatch().glProgramUniform2f(program, location, v0, v1); }
void glProgramUniform2fv(std::uint32_t program, std::int32_t location, std::int32_t count, const float * value) noexcept { return mygl::get_static_dispatch().glProgramUniform2fv(program, location, count, value); }
void glProgramUniform2i(std::uint32_t program, std::int32_t location, std::int32_t v0, std::int32_t v1) noexcept { return mygl::get_static_dispatch().glProgramUniform2i(program, location, v0, v1); }
void glProgramUniform2i64ARB(std::uint32_t program, std::int32_t location, std::int64_t x, std::int64_t y) noexcept { return mygl::get_static_dispatch().glProgramUniform2i64ARB(program, location, x, y); }
void glProgramUniform2i64NV(std::uint32_t program, std::int32_t location, std::int64_t x, std::int64_t y) noexcept { return mygl::get_static_dispatch().glProgramUniform2i64NV(program, location, x, y); }
void glProgramUniform2i64vARB(std::uint32_t program, std::int32_t location, std::int32_t count, const std::int64_t * value) noexcept { return mygl::get_static_dispatch().glProgramUniform2i64vARB(program, location, count, value); }
void glProgramUniform2i64vNV(std::uint32_t program, std::int32_t location, std::int32_t count, const std::int64_t * value) noexcept { return mygl::get_static_dispatch().glProgramUniform2i64vNV(program, location, count, value); }
void glProgramUniform2iv(std::uint32_t program, std::int32_t location, std::int32_t count, const std::int32_t * value) noexcept { return mygl::get_static_dispatch().glProgramUniform2iv(program, location, count, value); }
void glProgramUniform2ui(std::uint32_t program, std::int32_t location, std::uint32_t v0, std::uint32_t v1) noexcept { return mygl::get_static_dispatch().glProgramUniform2ui(program, location, v0, v1); }
void glProgramUniform2ui64ARB(std::uint32_t program, std::int32_t location, std::uint64_t x, std::uint64_t y) noexcept { return mygl::get_static_dispatch().glProgramUniform2ui64ARB(program, location, x, y); }
void glProgramUniform2ui64NV(std::uint32_t program, std::int32_t location, std::uint64_t x, std::uint64_t y) noexcept { return mygl::get_static_dispatch().glProgramUniform2ui64NV(program, location, x, y); }
void glProgramUniform2ui64vARB(std::uint32_t program, std::int32_t location, std::int32_t count, const std::uint64_t * value) noexcept { return mygl::get_static_dispatch().glProgramUniform2ui64vARB(program, location, count, value); }
void glProgramUniform2ui64vNV(std::uint32_t program, std::int32_t location, std::int32_t count, const std::uint64_t * value) noexcept { return mygl::get_static_dispatch().glProgramUniform2ui64vNV(program, location, count, value); }
void glProgramUniform2uiv(std::uint32_t program, std::int32_t location, std::int32_t count, const std::uint32_t * value) noexcept { return mygl::get_static_dispatch().glProgramUniform2uiv(program, location, count, value); }
void glProgramUniform3d(std::uint32_t program, std::int32_t location, double v0, double v1, double v2) noexcept { return mygl::get_static_dispatch().glProgramUniform3d(program, location, v0, v1, v2); }
void glProgramUniform3dv(std::uint32_t program, std::int32_t location, std::int32_t count, const double * value) noexcept { return mygl::get_static_dispatch().glProgramUniform3dv(program, location, count, value); }
void glProgramUniform3f(std::uint32_t program, std::int32_t location, float v0, float v1, float v2) noexcept { return mygl::get_static_dispatch().glProgramUniform3f(program, location, v0, v1, v2); }
void glProgramUniform3fv(std::uint32_t program, std::int32_t location, std::int32_t count, const float * value) noexcept { return mygl::get_static_dispatch().glProgramUniform3fv(program, location, count, value); }
void glProgramUniform3i(std::uint32_t program, std::int32_t location, std::int32_t v0, std::int32_t v1, std::int32_t v2) noexcept { return mygl::get_static_dispatch().glProgramUniform3i(program, location, v0, v1, v2); }
void glProgramUniform3i64ARB(std::uint32_t program, std::int32_t location, std::int64_t x, std::int64_t y, std::int64_t z) noexcept { return mygl::get_static_dispatch().glProgramUniform3i64ARB(program, location, x, y, z); }
void glProgramUniform3i64NV(std::uint32_t program, std::int32_t location, std::int64_t x, std::int64_t y, std::int64_t z) noexcept { return mygl::get_static_dispatch().glProgramUniform3i64NV(program, location, x, y, z); }
void glProgramUniform3i64vARB(std::uint32_t program, std::int32_t location, std::int32_t count, const std::int64_t * value) noexcept { return mygl::get_static_dispatch().glProgramUniform3i64vARB(program, location, count, value); }
void glProgramUniform3i64vNV(std::uint32_t program, std::int32_t location, std::int32_t count, const std::int64_t * value) noexcept { return mygl::get_static_dispatch().glProgramUniform3i64vNV(program, location, count, value); }
void glProgramUniform3iv(std::uint32_t program, std::int32_t location, std::int32_t count, const std::int32_t * value) noexcept { return mygl::get_static_dispatch().glProgramUniform3iv(program, location, count, value); }
void glProgramUniform3ui(std::uint32_t program, std::int32_t location, std::uint32_t v0, std::uint32_t v1, std::uint32_t v2) noexcept { return mygl::get_static_dispatch().glProgramUniform3ui(program, location, v0, v1, v2); }
void glProgramUniform3ui64ARB(std::uint32_t program, std::int32_t location, std::uint64_t x, std::uint64_t y, std::uint64_t z) noexcept { return mygl::get_static_dispatch().glProgramUniform3ui64ARB(program, location, x, y, z); }
void glProgramUniform3ui64NV(std::uint32_t program, std::int32_t location, std::uint64_t x, std::uint64_t y, std::uint64_t z) noexcept { return mygl::get_static_dispatch().glProgramUniform3ui64NV(program, location, x, y, z); }
void glProgramUniform3ui64vARB(std::uint32_t program, std::int32_t location, std::int32_t count, const std::uint64_t * value) noexcept { return mygl::get_static_dispatch().glProgramUniform3ui64vARB(program, location, count, value); }
void glProgramUniform3ui64vNV(std::uint32_t program, std::int32_t location, std::int32_t count, const std::uint64_t * value) noexcept { return mygl::get_static_dispatch().glProgramUniform3ui64vNV(program, location, count, value); }
void glProgramUniform3uiv(std::uint32_t program, std::int32_t location, std::int32_t count, const std::uint32_t * value) noexcept { return mygl::get_static_dispatch().glProgramUniform3uiv(program, location, count, value); }
void glProgramUniform4d(std::uint32_t program, std::int32_t location, double v0, double v1, double v2, double v3) noexcept { return mygl::get_static_dispatch().glProgramUniform4d(program, location, v0, v1, v2, v3); }
void glProgramUniform4dv(std::uint32_t program, std::int32_t location, std::int32_t count, const double * value) noexcept { return mygl::get_static_dispatch().glProgramUniform4dv(program, location, count, value); }
void glProgramUniform4f(std::uint32_t program, std::int32_t location, float v0, float v1, float v2, float v3) noexcept { return mygl::get_static_dispatch().glProgramUniform4f(program, location, v0, v1, v2, v3); }
void glProgramUniform4fv(std::uint32_t program, std::int32_t location, std::int32_t count, const float * value) noexcept { return mygl::get_static_dispatch().glProgramUniform4fv(program, location, count, value); }
void glProgramUniform4i(std::uint32_t program, std::int32_t location, std::int32_t v0, std::int32_t v1, std::int32_t v2, std::int32_t v3) noexcept { return mygl::get_static_dispatch().glProgramUniform4i(program, location, v0, v1, v2, v3); }
void glProgramUniform4i64ARB(std::uint32_t program, std::int32_t location, std::int64_t x, std::int64_t y, std::int64_t z, std::int64_t w) noexcept { return mygl::get_static_dispatch().glProgramUniform4i64ARB(program, location, x, y, z, w); }
void glProgramUniform4i64NV(std::uint32_t program, std::int32_t location, std::int64_t x, std::int64_t y, std::int64_t z, std::int64_t w) noexcept { return mygl::get_static_dispatch().glProgramUniform4i64NV(program, location, x, y, z, w); }
void glProgramUniform4i64vARB(std::uint32_t program, std::int32_t location, std::int32_t count, const std::int64_t * value) noexcept { return mygl::get_static_dispatch().glProgramUniform4i64vARB(program, location, count, value); }
void glProgramUniform4i64vNV(std::uint32_t program, std::int32_t location, std::int32_t count, const std::int64_t * value) noexcept { return mygl::get_static_dispatch().glProgramUniform4i64vNV(program, location, count, value); }
void glProgramUniform4iv(std::uint32_t program, std::int32_t location, std::int32_t count, const std::int32_t * value) noexcept { return mygl::get_static_dispatch().glProgramUniform4iv(program, location, count, value); }
void glProgramUniform4ui(std::uint32_t program, std::int32_t location, std::uint32_t v0, std::uint32_t v1, std::uint32_t v2, std::uint32_t v3) noexcept { return mygl::get_static_dispatch().glProgramUniform4ui(program, location, v0, v1, v2, v3); }
void glProgramUniform4ui64ARB(std::uint32_t program, std::int32_t location, std::uint64_t x, std::uint64_t y, std::uint64_t z, std::uint64_t w) noexcept { return mygl::get_static_dispatch().glProgramUniform4ui64ARB(program, location, x, y, z, w); }
void glProgramUniform4ui64NV(std::uint32_t program, std::int32_t location, std::uint64_t x, std::uint64_t y, std::uint64_t z, std::uint64_t w) noexcept { return mygl::get_static_dispatch().glProgramUniform4ui64NV(program, location, x, y, z, w); }
void glProgramUniform4ui64vARB(std::uint32_t program, std::int32_t location, std::int32_t count, const std::uint64_t * value) noexcept { return mygl::get_static_dispatch().glProgramUniform4ui64vARB(program, location, count, value); }
void glProgramUniform4ui64vNV(std::uint32_t program, std::int32_t location, std::int32_t count, const std::uint64_t * value) noexcept { return mygl::get_static_dispatch().glProgramUniform4ui64vNV(program, location, count, value); }
void glProgramUniform4uiv(std::uint32_t program, std::int32_t location, std::int32_t count, const std::uint32_t * value) noexcept { return mygl::get_static_dispatch().glProgramUniform4uiv(program, location, count, value); }
void glProgramUniformHandleui64ARB(std::uint32_t program, std::int32_t location, std::uint64_t value) noexcept { return mygl::get_static_dispatch().glProgramUniformHandleui64ARB(program, location, value); }
void glProgramUniformHandleui64vARB(std::uint32_t program, std::int32_t location, std::int32_t count, const std::uint64_t * values) noexcept { return mygl::get_static_dispatch().glProgramUniformHandleui64vARB(program, location, count, values); }
void glProgramUniformMatrix2dv(std::uint32_t program, std::int32_t location, std::int32_t count, bool transpose, const double * value) noexcept { return mygl::get_static_dispatch().glProgramUniformMatrix2dv(program, location, count, transpose, value); }
void glProgramUniformMatrix2fv(std::uint32_t program, std::int32_t location, std::int32_t count, bool transpose, const float * value) noexcept { return mygl::get_static_dispatch().glProgramUniformMatrix2fv(program, location, count, transpose, value); }
void glProgramUniformMatrix2x3dv(std::uint32_t program, std::int32_t location, std::int32_t count, bool transpose, const double * value) noexcept { return mygl::get_static_dispatch().glProgramUniformMatrix2x3dv(program, location, count, transpose, value); }
void glProgramUniformMatrix2x3fv(std::uint32_t program, std::int32_t location, std::int32_t count, bool transpose, const float * value) noexcept { return mygl::get_static_dispatch().glProgramUniformMatrix2x3fv(program, location, count, transpose, value); }
void glProgramUniformMatrix2x4dv(std::uint32_t program, std::int32_t location, std::int32_t count, bool transpose, const double * value) noexcept { return mygl::get_static_dispatch().glProgramUniformMatrix2x4dv(program, location, count, transpose, value); }
void glProgramUniformMatrix2x4fv(std::uint32_t program, std::int32_t location, std::int32_t count, bool transpose, const float * value) noexcept { return mygl::get_static_dispatch().glProgramUniformMatrix2x4fv(program, location, count, transpose, value); }
void glProgramUniformMatrix3dv(std::uint32_t program, std::int32_t location, std::int32_t count, bool transpose, const double * value) noexcept { return mygl::get_static_dispatch().glProgramUniformMatrix3dv(program, location, count, transpose, value); }
void glProgramUniformMatrix3fv(std::uint32_t program, std::int32_t location, std::int32_t count, bool transpose, const float * value) noexcept { return mygl::get_static_dispatch().glProgramUniformMatrix3fv(program, location, count, transpose, value); }
void glProgramUniformMatrix3x2dv(std::uint32_t program, std::int32_t location, std::int32_t count, bool transpose, const double * value) noexcept { return mygl::get_static_dispatch().glProgramUniformMatrix3x2dv(program, location, count, transpose, value); }
void glProgramUniformMatrix3x2fv(std::uint32_t program, std::int32_t location, std::int32_t count, bool transpose, const float * value) noexcept { return mygl::get_static_dispatch().glProgramUniformMatrix3x2fv(program, location, count, transpose, value); }
void glProgramUniformMatrix3x4dv(std::uint32_t program, std::int32_t location, std::int32_t count, bool transpose, const double * value) noexcept { return mygl::get_static_dispatch().glProgramUniformMatrix3x4dv(program, location, count, transpose, value); }
void glProgramUniformMatrix3x4fv(std::uint32_t program, std::int32_t location, std::int32_t count, bool transpose, const float * value) noexcept { return mygl::get_static_dispatch().glProgramUniformMatrix3x4fv(program, location, count, transpose, value); }
void glProgramUniformMatrix4dv(std::uint32_t program, std::int32_t location, std::int32_t count, bool transpose, const double * value) noexcept { return mygl::get_static_dispatch().glProgramUniformMatrix4dv(program, location, count, transpose, value); }
void glProgramUniformMatrix4fv(std::uint32_t program, std::int32_t location, std::int32_t count, bool transpose, const float * value) noexcept { return mygl::get_static_dispatch().glProgramUniformMatrix4fv(program, location, count, transpose, value); }
void glProgramUniformMatrix4x2dv(std::uint32_t program, std::int32_t location, std::int32_t count, bool transpose, const double * value) noexcept { return mygl::get_static_dispatch().glProgramUniformMatrix4x2dv(program, location, count, transpose, value); }
void glProgramUniformMatrix4x2fv(std::uint32_t program, std::int32_t location, std::int32_t count, bool transpose, const float * value) noexcept { return mygl::get_static_dispatch().glProgramUniformMatrix4x2fv(program, location, count, transpose, value); }
void glProgramUniformMatrix4x3dv(std::uint32_t program, std::int32_t location, std::int32_t count, bool transpose, const double * value) noexcept { return mygl::get_static_dispatch().glProgramUniformMatrix4x3dv(program, location, count, transpose, value); }
void glProgramUniformMatrix4x3fv(std::uint32_t program, std::int32_t location, std::int32_t count, bool transpose, const float * value) noexcept { return mygl::get_static_dispatch().glProgramUniformMatrix4x3fv(program, location, count, transpose, value); }
void glProgramUniformui64NV(std::uint32_t program, std::int32_t location, std::uint64_t value) noexcept { return mygl::get_static_dispatch().glProgramUniformui64NV(program, location, value); }
void glProgramUniformui64vNV(std::uint32_t program, std::int32_t location, std::int32_t count, const std::uint64_t * value) noexcept { return mygl::get_static_dispatch().glProgramUniformui64vNV(program, location, count, value); }
void glProvokingVertex(GLenum mode) noexcept { return mygl::get_static_dispatch().glProvokingVertex(mode); }
void glPushDebugGroup(GLenum source, std::uint32_t id, std::int32_t length, const char * message) noexcept { return mygl::get_static_dispatch().glPushDebugGroup(source, id, length, message); }
void glPushDebugGroupKHR(GLenum source, std::uint32_t id, std::int32_t length, const char * message) noexcept { return mygl::get_static_dispatch().glPushDebugGroupKHR(source, id, length, message); }
void glQueryCounter(std::uint32_t id, GLenum target) noexcept { return mygl::get_static_dispatch().glQueryCounter(id, target); }
void glReadBuffer(GLenum src) noexcept { return mygl::get_static_dispatch().glReadBuffer(src); }
void glReadPixels(std::int32_t x, std::int32_t y, std::int32_t width, std::int32_t height, GLenum format, GLenum type, void * pixels) noexcept { return mygl::get_static_dispatch().glReadPixels(x, y, width, height, format, type, pixels); }
void glReadnPixels(std::int32_t x, std::int32_t y, std::int32_t width, std::int32_t height, GLenum format, GLenum type, std::int32_t bufSize, void * data) noexcept { return mygl::get_static_dispatch().glReadnPixels(x, y, width, height, format, type, bufSize, data); }
void glReleaseShaderCompiler() noexcept { return mygl::get_static_dispatch().glReleaseShaderCompiler(); }
void glRenderbufferStorage(GLenum target, GLenum internalformat, std::int32_t width, std::int32_t height) noexcept { return mygl::get_static_dispatch().glRenderbufferStorage(target, internalformat, width, height); }
void glRenderbufferStorageMultisample(GLenum target, std::int32_t samples, GLenum internalformat, std::int32_t width, std::int32_t height) noexcept { return mygl::get_static_dispatch().glRenderbufferStorageMultisample(target, samples, internalformat, width, height); }
void glResumeTransformFeedback() noexcept { return mygl::get_static_dispatch().glResumeTransformFeedback(); }
void glSampleCoverage(float value, bool invert) noexcept { return mygl::get_static_dispatch().glSampleCoverage(value, invert); }
void glSampleMaski(std::uint32_t maskNumber, GLbitfield mask) noexcept { return mygl::get_static_dispatch().glSampleMaski(maskNumber, mask); }
void glSamplerParameterIiv(std::uint32_t sampler, GLenum pname, const std::int32_t * param) noexcept { return mygl::get_static_dispatch().glSamplerParameterIiv(sampler, pname, param); }
void glSamplerParameterIuiv(std::uint32_t sampler, GLenum pname, const std::uint32_t * param) noexcept { return mygl::get_static_dispatch().glSamplerParameterIuiv(sampler, pname, param); }
void glSamplerParameterf(std::uint32_t sampler, GLenum pname, float param) noexcept { return mygl::get_static_dispatch().glSamplerParameterf(sampler, pname, param); }
void glSamplerParameterfv(std::uint32_t sampler, GLenum pname, const float * param) noexcept { return mygl::get_static_dispatch().glSamplerParameterfv(sampler, pname, param); }
void glSamplerParameteri(std::uint32_t sampler, GLenum pname, std::int32_t param) noexcept { return mygl::get_static_dispatch().glSamplerParameteri(sampler, pname, param); }
void glSamplerParameteriv(std::uint32_t sampler, GLenum pname, const std::int32_t * param) noexcept { return mygl::get_static_dispatch().glSamplerParameteriv(sampler, pname, param); }
void glScissor(std::int32_t x, std::int32_t y, std::int32_t width, std::int32_t height) noexcept { return mygl::get_static_dispatch().glScissor(x, y, width, height); }
void glScissorArrayv(std::uint32_t first, std::int32_t count, const std::int32_t * v) noexcept { return mygl::get_static_dispatch().glScissorArrayv(first, count, v); }
void glScissorIndexed(std::uint32_t index, std::int32_t left, std::int32_t bottom, std::int32_t width, std::int32_t height) noexcept { return mygl::get_static_dispatch().glScissorIndexed(index, left, bottom, width, height); }
void glScissorIndexedv(std::uint32_t index, const std::int32_t * v) noexcept { return mygl::get_static_dispatch().glScissorIndexedv(index, v); }
void glSecondaryColorFormatNV(std::int32_t size, GLenum type, std::int32_t stride) noexcept { return mygl::get_static_dispatch().glSecondaryColorFormatNV(size, type, stride); }
void glSecondaryColorP3ui(GLenum type, std::uint32_t color) noexcept { return mygl::get_static_dispatch().glSecondaryColorP3ui(type, color); }
void glSecondaryColorP3uiv(GLenum type, const std::uint32_t * color) noexcept { return mygl::get_static_dispatch().glSecondaryColorP3uiv(type, color); }
void glSemaphoreParameterui64vEXT(std::uint32_t semaphore, GLenum pname, const std::uint64_t * params) noexcept { return mygl::get_static_dispatch().glSemaphoreParameterui64vEXT(semaphore, pname, params); }
void glShaderBinary(std::int32_t count, const std::uint32_t * shaders, GLenum binaryFormat, const void * binary, std::int32_t length) noexcept { return mygl::get_static_dispatch().glShaderBinary(count, shaders, binaryFormat, binary, length); }
void glShaderSource(std::uint32_t shader, std::int32_t count, const char *const* string, const std::int32_t * length) noexcept { return mygl::get_static_dispatch().glShaderSource(shader, count, string, length); }
void glShaderStorageBlockBinding(std::uint32_t program, std::uint32_t storageBlockIndex, std::uint32_t storageBlockBinding) noexcept { return mygl::get_static_dispatch().glShaderStorageBlockBinding(program, storageBlockIndex, storageBlockBinding); }
void glSignalSemaphoreEXT(std::uint32_t semaphore, std::uint32_t numBufferBarriers, const std::uint32_t * buffers, std::uint32_t numTextureBarriers, const std::uint32_t * textures, const GLenum * dstLayouts) noexcept { return mygl::get_static_dispatch().glSignalSemaphoreEXT(semaphore, numBufferBarriers, buffers, numTextureBarriers, textures, dstLayouts); }
void glSpecializeShader(std::uint32_t shader, const char * pEntryPoint, std::uint32_t numSpecializationConstants, const std::uint32_t * pConstantIndex, const std::uint32_t * pConstantValue) noexcept { return mygl::get_static_dispatch().glSpecializeShader(shader, pEntryPoint, numSpecializationConstants, pConstantIndex, pConstantValue); }
void glStateCaptureNV(std::uint32_t state, GLenum mode) noexcept { return mygl::get_static_dispatch().glStateCaptureNV(state, mode); }
void glStencilFillPathInstancedNV(std::int32_t numPaths, GLenum pathNameType, const void * paths, std::uint32_t pathBase, GLenum fillMode, std::uint32_t mask, GLenum transformType, const float * transformValues) noexcept { return mygl::get_static_dispatch().glStencilFillPathInstancedNV(numPaths, pathNameType, paths, pathBase, fillMode, mask, transformType, transformValues); }
void glStencilFillPathNV(std::uint32_t path, GLenum fillMode, std::uint32_t mask) noexcept { return mygl::get_static_dispatch().glStencilFillPathNV(path, fillMode, mask); }
void glStencilFunc(GLenum func, std::int32_t ref, std::uint32_t mask) noexcept { return mygl::get_static_dispatch().glStencilFunc(func, ref, mask); }
void glStencilFuncSeparate(GLenum face, GLenum func, std::int32_t ref, std::uint32_t mask) noexcept { return mygl::get_static_dispatch().glStencilFuncSeparate(face, func, ref, mask); }
void glStencilMask(std::uint32_t mask) noexcept { return mygl::get_static_dispatch().glStencilMask(mask); }
void glStencilMaskSeparate(GLenum face, std::uint32_t mask) noexcept { return mygl::get_static_dispatch().glStencilMaskSeparate(face, mask); }
void glStencilOp(GLenum fail, GLenum zfail, GLenum zpass) noexcept { return mygl::get_static_dispatch().glStencilOp(fail, zfail, zpass); }
void glStencilOpSeparate(GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass) noexcept { return mygl::get_static_dispatch().glStencilOpSeparate(face, sfail, dpfail, dppass); }
void glStencilStrokePathInstancedNV(std::int32_t numPaths, GLenum pathNameType, const void * paths, std::uint32_t pathBase, std::int32_t reference, std::uint32_t mask, GLenum transformType, const float * transformValues) noexcept { return mygl::get_static_dispatch().glStencilStrokePathInstancedNV(numPaths, pathNameType, paths, pathBase, reference, mask, transformType, transformValues); }
void glStencilStrokePathNV(std::uint32_t path, std::int32_t reference, std::uint32_t mask) noexcept { return mygl::get_static_dispatch().glStencilStrokePathNV(path, reference, mask); }
void glStencilThenCoverFillPathInstancedNV(std::int32_t numPaths, GLenum pathNameType, const void * paths, std::uint32_t pathBase, GLenum fillMode, std::uint32_t mask, GLenum coverMode, GLenum transformType, const float * transformValues) noexcept { return mygl::get_static_dispatch().glStencilThenCoverFillPathInstancedNV(numPaths, pathNameType, paths, pathBase, fillMode, mask, coverMode, transformType, transformValues); }
void glStencilThenCoverFillPathNV(std::uint32_t path, GLenum fillMode, std::uint32_t mask, GLenum coverMode) noexcept { return mygl::get_static_dispatch().glStencilThenCoverFillPathNV(path, fillMode, mask, coverMode); }
void glStencilThenCoverStrokePathInstancedNV(std::int32_t numPaths, GLenum pathNameType, const void * paths, std::uint32_t pathBase, std::int32_t reference, std::uint32_t mask, GLenum coverMode, GLenum transformType, const float * transformValues) noexcept { return mygl::get_static_dispatch().glStencilThenCoverStrokePathInstancedNV(numPaths, pathNameType, paths, pathBase, reference, mask, coverMode, transformType, transformValues); }
void glStencilThenCoverStrokePathNV(std::uint32_t path, std::int32_t reference, std::uint32_t mask, GLenum coverMode) noexcept { return mygl::get_static_dispatch().glStencilThenCoverStrokePathNV(path, reference, mask, coverMode); }
void glTexBuffer(GLenum target, GLenum internalformat, std::uint32_t buffer) noexcept { return mygl::get_static_dispatch().glTexBuffer(target, internalformat, buffer); }
void glTexBufferRange(GLenum target, GLenum internalformat, std::uint32_t buffer, std::intptr_t offset, std::int64_t size) noexcept { return mygl::get_static_dispatch().glTexBufferRange(target, internalformat, buffer, offset, size); }
void glTexCoordFormatNV(std::int32_t size, GLenum type, std::int32_t stride) noexcept { return mygl::get_static_dispatch().glTexCoordFormatNV(size, type, stride); }
void glTexCoordP1ui(GLenum type, std::uint32_t coords) noexcept { return mygl::get_static_dispatch().glTexCoordP1ui(type, coords); }
void glTexCoordP1uiv(GLenum type, const std::uint32_t * coords) noexcept { return mygl::get_static_dispatch().glTexCoordP1uiv(type, coords); }
void glTexCoordP2ui(GLenum type, std::uint32_t coords) noexcept { return mygl::get_static_dispatch().glTexCoordP2ui(type, coords); }
void glTexCoordP2uiv(GLenum type, const std::uint32_t * coords) noexcept { return mygl::get_static_dispatch().glTexCoordP2uiv(type, coords); }
void glTexCoordP3ui(GLenum type, std::uint32_t coords) noexcept { return mygl::get_static_dispatch().glTexCoordP3ui(type, coords); }
void glTexCoordP3uiv(GLenum type, const std::uint32_t * coords) noexcept { return mygl::get_static_dispatch().glTexCoordP3uiv(type, coords); }
void glTexCoordP4ui(GLenum type, std::uint32_t coords) noexcept { return mygl::get_static_dispatch().glTexCoordP4ui(type, coords); }
void glTexCoordP4uiv(GLenum type, const std::uint32_t * coords) noexcept { return mygl::get_static_dispatch().glTexCoordP4uiv(type, coords); }
void glTexImage1D(GLenum target, std::int32_t level, std::int32_t internalformat, std::int32_t width, std::int32_t border, GLenum format, GLenum type, const void * pixels) noexcept { return mygl::get_static_dispatch().glTexImage1D(target, level, internalformat, width, border, format, type, pixels); }
void glTexImage2D(GLenum target, std::int32_t level, std::int32_t internalformat, std::int32_t width, std::int32_t height, std::int32_t border, GLenum format, GLenum type, const void * pixels) noexcept { return mygl::get_static_dispatch().glTexImage2D(target, level, internalformat, width, height, border, format, type, pixels); }
void glTexImage2DMultisample(GLenum target, std::int32_t samples, GLenum internalformat, std::int32_t width, std::int32_t height, bool fixedsamplelocations) noexcept { return mygl::get_static_dispatch().glTexImage2DMultisample(target, samples, internalformat, width, height, fixedsamplelocations); }
void glTexImage3D(GLenum target, std::int32_t level, std::int32_t internalformat, std::int32_t width, std::int32_t height, std::int32_t depth, std::int32_t border, GLenum format, GLenum type, const void * pixels) noexcept { return mygl::get_static_dispatch().glTexImage3D(target, level, internalformat, width, height, depth, border, format, type, pixels); }
void glTexImage3DMultisample(GLenum target, std::int32_t samples, GLenum internalformat, std::int32_t width, std::int32_t height, std::int32_t depth, bool fixedsamplelocations) noexcept { return mygl::get_static_dispatch().glTexImage3DMultisample(target, samples, internalformat, width, height, depth, fixedsamplelocations); }
void glTexParameterIiv(GLenum target, GLenum pname, const std::int32_t * params) noexcept { return mygl::get_static_dispatch().glTexParameterIiv(target, pname, params); }
void glTexParameterIuiv(GLenum target, GLenum pname, const std::uint32_t * params) noexcept { return mygl::get_static_dispatch().glTexParameterIuiv(target, pname, params); }
void glTexParameterf(GLenum target, GLenum pname, float param) noexcept { return mygl::get_static_dispatch().glTexParameterf(target, pname, param); }
void glTexParameterfv(GLenum target, GLenum pname, const float * params) noexcept { return mygl::get_static_dispatch().glTexParameterfv(target, pname, params); }
void glTexParameteri(GLenum target, GLenum pname, std::int32_t param) noexcept { return mygl::get_static_dispatch().glTexParameteri(target, pname, param); }
void glTexParameteriv(GLenum target, GLenum pname, const std::int32_t * params) noexcept { return mygl::get_static_dispatch().glTexParameteriv(target, pname, params); }
void glTexStorage1D(GLenum target, std::int32_t levels, GLenum internalformat, std::int32_t width) noexcept { return mygl::get_static_dispatch().glTexStorage1D(target, levels, internalformat, width); }
void glTexStorage2D(GLenum target, std::int32_t levels, GLenum internalformat, std::int32_t width, std::int32_t height) noexcept { return mygl::get_static_dispatch().glTexStorage2D(target, levels, internalformat, width, height); }
void glTexStorage2DMultisample(GLenum target, std::int32_t samples, GLenum internalformat, std::int32_t width, std::int32_t height, bool fixedsamplelocations) noexcept { return mygl::get_static_dispatch().glTexStorage2DMultisample(target, samples, internalformat, width, height, fixedsamplelocations); }
void glTexStorage3D(GLenum target, std::int32_t levels, GLenum internalformat, std::int32_t width, std::int32_t height, std::int32_t depth) noexcept { return mygl::get_static_dispatch().glTexStorage3D(target, levels, internalformat, width, height, depth); }
void glTexStorage3DMultisample(GLenum target, std::int32_t samples, GLenum internalformat, std::int32_t width, std::int32_t height, std::int32_t depth, bool fixedsamplelocations) noexcept { return mygl::get_static_dispatch().glTexStorage3DMultisample(target, samples, internalformat, width, height, depth, fixedsamplelocations); }
void glTexStorageMem1DEXT(GLenum target, std::int32_t levels, GLenum internalFormat, std::int32_t width, std::uint32_t memory, std::uint64_t offset) noexcept { return mygl::get_static_dispatch().glTexStorageMem1DEXT(target, levels, internalFormat, width, memory, offset); }
void glTexStorageMem2DEXT(GLenum target, std::int32_t levels, GLenum internalFormat, std::int32_t width, std::int32_t height, std::uint32_t memory, std::uint64_t offset) noexcept { return mygl::get_static_dispatch().glTexStorageMem2DEXT(target, levels, internalFormat, width, height, memory, offset); }
void glTexStorageMem2DMultisampleEXT(GLenum target, std::int32_t samples, GLenum internalFormat, std::int32_t width, std::int32_t height, bool fixedSampleLocations, std::uint32_t memory, std::uint64_t offset) noexcept { return mygl::get_static_dispatch().glTexStorageMem2DMultisampleEXT(target, samples, internalFormat, width, height, fixedSampleLocations, memory, offset); }
void glTexStorageMem3DEXT(GLenum target, std::int32_t levels, GLenum internalFormat, std::int32_t width, std::int32_t height, std::int32_t depth, std::uint32_t memory, std::uint64_t offset) noexcept { return mygl::get_static_dispatch().glTexStorageMem3DEXT(target, levels, internalFormat, width, height, depth, memory, offset); }
void glTexStorageMem3DMultisampleEXT(GLenum target, std::int32_t samples, GLenum internalFormat, std::int32_t width, std::int32_t height, std::int32_t depth, bool fixedSampleLocations, std::uint32_t memory, std::uint64_t offset) noexcept { return mygl::get_static_dispatch().glTexStorageMem3DMultisampleEXT(target, samples, internalFormat, width, height, depth, fixedSampleLocations, memory, offset); }
void glTexSubImage1D(GLenum target, std::int32_t level, std::int32_t xoffset, std::int32_t width, GLenum format, GLenum type, const void * pixels) noexcept { return mygl::get_static_dispatch().glTexSubImage1D(target, level, xoffset, width, format, type, pixels); }
void glTexSubImage2D(GLenum target, std::int32_t level, std::int32_t xoffset, std::int32_t yoffset, std::int32_t width, std::int32_t height, GLenum format, GLenum type, const void * pixels) noexcept { return mygl::get_static_dispatch().glTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels); }
void glTexSubImage3D(GLenum target, std::int32_t level, std::int32_t xoffset, std::int32_t yoffset, std::int32_t zoffset, std::int32_t width, std::int32_t height, std::int32_t depth, GLenum format, GLenum type, const void * pixels) noexcept { return mygl::get_static_dispatch().glTexSubImage3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels); }
void glTextureBarrier() noexcept { return mygl::get_static_dispatch().glTextureBarrier(); }
void glTextureBuffer(std::uint32_t texture, GLenum internalformat, std::uint32_t buffer) noexcept { return mygl::get_static_dispatch().glTextureBuffer(texture, internalformat, buffer); }
void glTextureBufferRange(std::uint32_t texture, GLenum internalformat, std::uint32_t buffer, std::intptr_t offset, std::int64_t size) noexcept { return mygl::get_static_dispatch().glTextureBufferRange(texture, internalformat, buffer, offset, size); }
void glTextureParameterIiv(std::uint32_t texture, GLenum pname, const std::int32_t * params) noexcept { return mygl::get_static_dispatch().glTextureParameterIiv(texture, pname, params); }
void glTextureParameterIuiv(std::uint32_t texture, GLenum pname, const std::uint32_t * params) noexcept { return mygl::get_static_dispatch().glTextureParameterIuiv(texture, pname, params); }
void glTextureParameterf(std::uint32_t texture, GLenum pname, float param) noexcept { return mygl::get_static_dispatch().glTextureParameterf(texture, pname, param); }
void glTextureParameterfv(std::uint32_t texture, GLenum pname, const float * param) noexcept { return mygl::get_static_dispatch().glTextureParameterfv(texture, pname, param); }
void glTextureParameteri(std::uint32_t texture, GLenum pname, std::int32_t param) noexcept { return mygl::get_static_dispatch().glTextureParameteri(texture, pname, param); }
void glTextureParameteriv(std::uint32_t texture, GLenum pname, const std::int32_t * param) noexcept { return mygl::get_static_dispatch().glTextureParameteriv(texture, pname, param); }
void glTextureStorage1D(std::uint32_t texture, std::int32_t levels, GLenum internalformat, std::int32_t width) noexcept { return mygl::get_static_dispatch().glTextureStorage1D(texture, levels, internalformat, width); }
void glTextureStorage2D(std::uint32_t texture, std::int32_t levels, GLenum internalformat, std::int32_t width, std::int32_t height) noexcept { return mygl::get_static_dispatch().glTextureStorage2D(texture, levels, internalformat, width, height); }
void glTextureStorage2DMultisample(std::uint32_t texture, std::int32_t samples, GLenum internalformat, std::int32_t width, std::int32_t height, bool fixedsamplelocations) noexcept { return mygl::get_static_dispatch().glTextureStorage2DMultisample(texture, samples, internalformat, width, height, fixedsamplelocations); }
void glTextureStorage3D(std::uint32_t texture, std::int32_t levels, GLenum internalformat, std::int32_t width, std::int32_t height, std::int32_t depth) noexcept { return mygl::get_static_dispatch().glTextureStorage3D(texture, levels, internalformat, width, height, depth); }
void glTextureStorage3DMultisample(std::uint32_t texture, std::int32_t samples, GLenum internalformat, std::int32_t width, std::int32_t height, std::int32_t depth, bool fixedsamplelocations) noexcept { return mygl::get_static_dispatch().glTextureStorage3DMultisample(texture, samples, internalformat, width, height, depth, fixedsamplelocations); }
void glTextureStorageMem1DEXT(std::uint32_t texture, std::int32_t levels, GLenum internalFormat, std::int32_t width, std::uint32_t memory, std::uint64_t offset) noexcept { return mygl::get_static_dispatch().glTextureStorageMem1DEXT(texture, levels, internalFormat, width, memory, offset); }
void glTextureStorageMem2DEXT(std::uint32_t texture, std::int32_t levels, GLenum internalFormat, std::int32_t width, std::int32_t height, std::uint32_t memory, std::uint64_t offset) noexcept { return mygl::get_static_dispatch().glTextureStorageMem2DEXT(texture, levels, internalFormat, width, height, memory, offset); }
void glTextureStorageMem2DMultisampleEXT(std::uint32_t texture, std::int32_t samples, GLenum internalFormat, std::int32_t width, std::int32_t height, bool fixedSampleLocations, std::uint32_t memory, std::uint64_t offset) noexcept { return mygl::get_static_dispatch().glTextureStorageMem2DMultisampleEXT(texture, samples, internalFormat, width, height, fixedSampleLocations, memory, offset); }
void glTextureStorageMem3DEXT(std::uint32_t texture, std::int32_t levels, GLenum internalFormat, std::int32_t width, std::int32_t height, std::int32_t depth, std::uint32_t memory, std::uint64_t offset) noexcept { return mygl::get_static_dispatch().glTextureStorageMem3DEXT(texture, levels, internalFormat, width, height, depth, memory, offset); }
void glTextureStorageMem3DMultisampleEXT(std::uint32_t texture, std::int32_t samples, GLenum internalFormat, std::int32_t width, std::int32_t height, std::int32_t depth, bool fixedSampleLocations, std::uint32_t memory, std::uint64_t offset) noexcept { return mygl::get_static_dispatch().glTextureStorageMem3DMultisampleEXT(texture, samples, internalFormat, width, height, depth, fixedSampleLocations, memory, offset); }
void glTextureSubImage1D(std::uint32_t texture, std::int32_t level, std::int32_t xoffset, std::int32_t width, GLenum format, GLenum type, const void * pixels) noexcept { return mygl::get_static_dispatch().glTextureSubImage1D(texture, level, xoffset, width, format, type, pixels); }
void glTextureSubImage2D(std::uint32_t texture, std::int32_t level, std::int32_t xoffset, std::int32_t yoffset, std::int32_t width, std::int32_t height, GLenum format, GLenum type, const void * pixels) noexcept { return mygl::get_static_dispatch().glTextureSubImage2D(texture, level, xoffset, yoffset, width, height, format, type, pixels); }
void glTextureSubImage3D(std::uint32_t texture, std::int32_t level, std::int32_t xoffset, std::int32_t yoffset, std::int32_t zoffset, std::int32_t width, std::int32_t height, std::int32_t depth, GLenum format, GLenum type, const void * pixels) noexcept { return mygl::get_static_dispatch().glTextureSubImage3D(texture, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels); }
void glTextureView(std::uint32_t texture, GLenum target, std::uint32_t origtexture, GLenum internalformat, std::uint32_t minlevel, std::uint32_t numlevels, std::uint32_t minlayer, std::uint32_t numlayers) noexcept { return mygl::get_static_dispatch().glTextureView(texture, target, origtexture, internalformat, minlevel, numlevels, minlayer, numlayers); }
void glTransformFeedbackBufferBase(std::uint32_t xfb, std::uint32_t index, std::uint32_t buffer) noexcept { return mygl::get_static_dispatch().glTransformFeedbackBufferBase(xfb, index, buffer); }
void glTransformFeedbackBufferRange(std::uint32_t xfb, std::uint32_t index, std::uint32_t buffer, std::intptr_t offset, std::int64_t size) noexcept { return mygl::get_static_dispatch().glTransformFeedbackBufferRange(xfb, index, buffer, offset, size); }
void glTransformFeedbackVaryings(std::uint32_t program, std::int32_t count, const char *const* varyings, GLenum bufferMode) noexcept { return mygl::get_static_dispatch().glTransformFeedbackVaryings(program, count, varyings, bufferMode); }
void glTransformPathNV(std::uint32_t resultPath, std::uint32_t srcPath, GLenum transformType, const float * transformValues) noexcept { return mygl::get_static_dispatch().glTransformPathNV(resultPath, srcPath, transformType, transformValues); }
void glUniform1d(std::int32_t location, double x) noexcept { return mygl::get_static_dispatch().glUniform1d(location, x); }
void glUniform1dv(std::int32_t location, std::int32_t count, const double * value) noexcept { return mygl::get_static_dispatch().glUniform1dv(location, count, value); }
void glUniform1f(std::int32_t location, float v0) noexcept { return mygl::get_static_dispatch().glUniform1f(location, v0); }
void glUniform1fv(std::int32_t location, std::int32_t count, const float * value) noexcept { return mygl::get_static_dispatch().glUniform1fv(location, count, value); }
void glUniform1i(std::int32_t location, std::int32_t v0) noexcept { return mygl::get_static_dispatch().glUniform1i(location, v0); }
void glUniform1i64ARB(std::int32_t location, std::int64_t x) noexcept { return mygl::get_static_dispatch().glUniform1i64ARB(location, x); }
void glUniform1i64NV(std::int32_t location, std::int64_t x) noexcept { return mygl::get_static_dispatch().glUniform1i64NV(location, x); }
void glUniform1i64vARB(std::int32_t location, std::int32_t count, const std::int64_t * value) noexcept { return mygl::get_static_dispatch().glUniform1i64vARB(location, count, value); }
void glUniform1i64vNV(std::int32_t location, std::int32_t count, const std::int64_t * value) noexcept { return mygl::get_static_dispatch().glUniform1i64vNV(location, count, value); }
void glUniform1iv(std::int32_t location, std::int32_t count, const std::int32_t * value) noexcept { return mygl::get_static_dispatch().glUniform1iv(location, count, value); }
void glUniform1ui(std::int32_t location, std::uint32_t v0) noexcept { return mygl::get_static_dispatch().glUniform1ui(location, v0); }
void glUniform1ui64ARB(std::int32_t location, std::uint64_t x) noexcept { return mygl::get_static_dispatch().glUniform1ui64ARB(location, x); }
void glUniform1ui64NV(std::int32_t location, std::uint64_t x) noexcept { return mygl::get_static_dispatch().glUniform1ui64NV(location, x); }
void glUniform1ui64vARB(std::int32_t location, std::int32_t count, const std::uint64_t * value) noexcept { return mygl::get_static_dispatch().glUniform1ui64vARB(location, count, value); }
void glUniform1ui64vNV(std::int32_t location, std::int32_t count, const std::uint64_t * value) noexcept { return mygl::get_static_dispatch().glUniform1ui64vNV(location, count, value); }
void glUniform1uiv(std::int32_t location, std::int32_t count, const std::uint32_t * value) noexcept { return mygl::get_static_dispatch().glUniform1uiv(location, count, value); }
void glUniform2d(std::int32_t location, double x, double y) noexcept { return mygl::get_static_dispatch().glUniform2d(location, x, y); }
void glUniform2dv(std::int32_t location, std::int32_t count, const double * value) noexcept { return mygl::get_static_dispatch().glUniform2dv(location, count, value); }
void glUniform2f(std::int32_t location, float v0, float v1) noexcept { return mygl::get_static_dispatch().glUniform2f(location, v0, v1); }
void glUniform2fv(std::int32_t location, std::int32_t count, const float * value) noexcept { return mygl::get_static_dispatch().glUniform2fv(location, count, value); }
void glUniform2i(std::int32_t location, std::int32_t v0, std::int32_t v1) noexcept { return mygl::get_static_dispatch().glUniform2i(location, v0, v1); }
void glUniform2i64ARB(std::int32_t location, std::int64_t x, std::int64_t y) noexcept { return mygl::get_static_dispatch().glUniform2i64ARB(location, x, y); }
void glUniform2i64NV(std::int32_t location, std::int64_t x, std::int64_t y) noexcept { return mygl::get_static_dispatch().glUniform2i64NV(location, x, y); }
void glUniform2i64vARB(std::int32_t location, std::int32_t count, const std::int64_t * value) noexcept { return mygl::get_static_dispatch().glUniform2i64vARB(location, count, value); }
void glUniform2i64vNV(std::int32_t location, std::int32_t count, const std::int64_t * value) noexcept { return mygl::get_static_dispatch().glUniform2i64vNV(location, count, value); }
void glUniform2iv(std::int32_t location, std::int32_t count, const std::int32_t * value) noexcept { return mygl::get_static_dispatch().glUniform2iv(location, count, value); }
void glUniform2ui(std::int32_t location, std::uint32_t v0, std::uint32_t v1) noexcept { return mygl::get_static_dispatch().glUniform2ui(location, v0, v1); }
void glUniform2ui64ARB(std::int32_t location, std::uint64_t x, std::uint64_t y) noexcept { return mygl::get_static_dispatch().glUniform2ui64ARB(location, x, y); }
void glUniform2ui64NV(std::int32_t location, std::uint64_t x, std::uint64_t y) noexcept { return mygl::get_static_dispatch().glUniform2ui64NV(location, x, y); }
void glUniform2ui64vARB(std::int32_t location, std::int32_t count, const std::uint64_t * value) noexcept { return mygl::get_static_dispatch().glUniform2ui64vARB(location, count, value); }
void glUniform2ui64vNV(std::int32_t location, std::int32_t count, const std::uint64_t * value) noexcept { return mygl::get_static_dispatch().glUniform2ui64vNV(location, count, value); }
void glUniform2uiv(std::int32_t location, std::int32_t count, const std::uint32_t * value) noexcept { return mygl::get_static_dispatch().glUniform2uiv(location, count, value); }
void glUniform3d(std::int32_t location, double x, double y, double z) noexcept { return mygl::get_static_dispatch().glUniform3d(location, x, y, z); }
void glUniform3dv(std::int32_t location, std::int32_t count, const double * value) noexcept { return mygl::get_static_dispatch().glUniform3dv(location, count, value); }
void glUniform3f(std::int32_t location, float v0, float v1, float v2) noexcept { return mygl::get_static_dispatch().glUniform3f(location, v0, v1, v2); }
void glUniform3fv(std::int32_t location, std::int32_t count, const float * value) noexcept { return mygl::get_static_dispatch().glUniform3fv(location, count, value); }
void glUniform3i(std::int32_t location, std::int32_t v0, std::int32_t v1, std::int32_t v2) noexcept { return mygl::get_static_dispatch().glUniform3i(location, v0, v1, v2); }
void glUniform3i64ARB(std::int32_t location, std::int64_t x, std::int64_t y, std::int64_t z) noexcept { return mygl::get_static_dispatch().glUniform3i64ARB(location, x, y, z); }
void glUniform3i64NV(std::int32_t location, std::int64_t x, std::int64_t y, std::int64_t z) noexcept { return mygl::get_static_dispatch().glUniform3i64NV(location, x, y, z); }
void glUniform3i64vARB(std::int32_t location, std::int32_t count, const std::int64_t * value) noexcept { return mygl::get_static_dispatch().glUniform3i64vARB(location, count, value); }
void glUniform3i64vNV(std::int32_t location, std::int32_t count, const std::int64_t * value) noexcept { return mygl::get_static_dispatch().glUniform3i64vNV(location, count, value); }
void glUniform3iv(std::int32_t location, std::int32_t count, const std::int32_t * value) noexcept { return mygl::get_static_dispatch().glUniform3iv(location, count, value); }
void glUniform3ui(std::int32_t location, std::uint32_t v0, std::uint32_t v1, std::uint32_t v2) noexcept { return mygl::get_static_dispatch().glUniform3ui(location, v0, v1, v2); }
void glUniform3ui64ARB(std::int32_t location, std::uint64_t x, std::uint64_t y, std::uint64_t z) noexcept { return mygl::get_static_dispatch().glUniform3ui64ARB(location, x, y, z); }
void glUniform3ui64NV(std::int32_t location, std::uint64_t x, std::uint64_t y, std::uint64_t z) noexcept { return mygl::get_static_dispatch().glUniform3ui64NV(location, x, y, z); }
void glUniform3ui64vARB(std::int32_t location, std::int32_t count, const std::uint64_t * value) noexcept { return mygl::get_static_dispatch().glUniform3ui64vARB(location, count, value); }
void glUniform3ui64vNV(std::int32_t location, std::int32_t count, const std::uint64_t * value) noexcept { return mygl::get_static_dispatch().glUniform3ui64vNV(location, count, value); }
void glUniform3uiv(std::int32_t location, std::int32_t count, const std::uint32_t * value) noexcept { return mygl::get_static_dispatch().glUniform3uiv(location, count, value); }
void glUniform4d(std::int32_t location, double x, double y, double z, double w) noexcept { return mygl::get_static_dispatch().glUniform4d(location, x, y, z, w); }
void glUniform4dv(std::int32_t location, std::int32_t count, const double * value) noexcept { return mygl::get_static_dispatch().glUniform4dv(location, count, value); }
void glUniform4f(std::int32_t location, float v0, float v1, float v2, float v3) noexcept { return mygl::get_static_dispatch().glUniform4f(location, v0, v1, v2, v3); }
void glUniform4fv(std::int32_t location, std::int32_t count, const float * value) noexcept { return mygl::get_static_dispatch().glUniform4fv(location, count, value); }
void glUniform4i(std::int32_t location, std::int32_t v0, std::int32_t v1, std::int32_t v2, std::int32_t v3) noexcept { return mygl::get_static_dispatch().glUniform4i(location, v0, v1, v2, v3); }
void glUniform4i64ARB(std::int32_t location, std::int64_t x, std::int64_t y, std::int64_t z, std::int64_t w) noexcept { return mygl::get_static_dispatch().glUniform4i64ARB(location, x, y, z, w); }
void glUniform4i64NV(std::int32_t location, std::int64_t x, std::int64_t y, std::int64_t z, std::int64_t w) noexcept { return mygl::get_static_dispatch().glUniform4i64NV(location, x, y, z, w); }
void glUniform4i64vARB(std::int32_t location, std::int32_t count, const std::int64_t * value) noexcept { return mygl::get_static_dispatch().glUniform4i64vARB(location, count, value); }
void glUniform4i64vNV(std::int32_t location, std::int32_t count, const std::int64_t * value) noexcept { return mygl::get_static_dispatch().glUniform4i64vNV(location, count, value); }
void glUniform4iv(std::int32_t location, std::int32_t count, const std::int32_t * value) noexcept { return mygl::get_static_dispatch().glUniform4iv(location, count, value); }
void glUniform4ui(std::int32_t location, std::uint32_t v0, std::uint32_t v1, std::uint32_t v2, std::uint32_t v3) noexcept { return mygl::get_static_dispatch().glUniform4ui(location, v0, v1, v2, v3); }
void glUniform4ui64ARB(std::int32_t location, std::uint64_t x, std::uint64_t y, std::uint64_t z, std::uint64_t w) noexcept { return mygl::get_static_dispatch().glUniform4ui64ARB(location, x, y, z, w); }
void glUniform4ui64NV(std::int32_t location, std::uint64_t x, std::uint64_t y, std::uint64_t z, std::uint64_t w) noexcept { return mygl::get_static_dispatch().glUniform4ui64NV(location, x, y, z, w); }
void glUniform4ui64vARB(std::int32_t location, std::int32_t count, const std::uint64_t * value) noexcept { return mygl::get_static_dispatch().glUniform4ui64vARB(location, count, value); }
void glUniform4ui64vNV(std::int32_t location, std::int32_t count, const std::uint64_t * value) noexcept { return mygl::get_static_dispatch().glUniform4ui64vNV(location, count, value); }
void glUniform4uiv(std::int32_t location, std::int32_t count, const std::uint32_t * value) noexcept { return mygl::get_static_dispatch().glUniform4uiv(location, count, value); }
void glUniformBlockBinding(std::uint32_t program, std::uint32_t uniformBlockIndex, std::uint32_t uniformBlockBinding) noexcept { return mygl::get_static_dispatch().glUniformBlockBinding(program, uniformBlockIndex, uniformBlockBinding); }
void glUniformHandleui64ARB(std::int32_t location, std::uint64_t value) noexcept { return mygl::get_static_dispatch().glUniformHandleui64ARB(location, value); }
void glUniformHandleui64vARB(std::int32_t location, std::int32_t count, const std::uint64_t * value) noexcept { return mygl::get_static_dispatch().glUniformHandleui64vARB(location, count, value); }
void glUniformMatrix2dv(std::int32_t location, std::int32_t count, bool transpose, const double * value) noexcept { return mygl::get_static_dispatch().glUniformMatrix2dv(location, count, transpose, value); }
void glUniformMatrix2fv(std::int32_t location, std::int32_t count, bool transpose, const float * value) noexcept { return mygl::get_static_dispatch().glUniformMatrix2fv(location, count, transpose, value); }
void glUniformMatrix2x3dv(std::int32_t location, std::int32_t count, bool transpose, const double * value) noexcept { return mygl::get_static_dispatch().glUniformMatrix2x3dv(location, count, transpose, value); }
void glUniformMatrix2x3fv(std::int32_t location, std::int32_t count, bool transpose, const float * value) noexcept { return mygl::get_static_dispatch().glUniformMatrix2x3fv(location, count, transpose, value); }
void glUniformMatrix2x4dv(std::int32_t location, std::int32_t count, bool transpose, const double * value) noexcept { return mygl::get_static_dispatch().glUniformMatrix2x4dv(location, count, transpose, value); }
void glUniformMatrix2x4fv(std::int32_t location, std::int32_t count, bool transpose, const float * value) noexcept { return mygl::get_static_dispatch().glUniformMatrix2x4fv(location, count, transpose, value); }
void glUniformMatrix3dv(std::int32_t location, std::int32_t count, bool transpose, const double * value) noexcept { return mygl::get_static_dispatch().glUniformMatrix3dv(location, count, transpose, value); }
void glUniformMatrix3fv(std::int32_t location, std::int32_t count, bool transpose, const float * value) noexcept { return mygl::get_static_dispatch().glUniformMatrix3fv(location, count, transpose, value); }
void glUniformMatrix3x2dv(std::int32_t location, std::int32_t count, bool transpose, const double * value) noexcept { return mygl::get_static_dispatch().glUniformMatrix3x2dv(location, count, transpose, value); }
void glUniformMatrix3x2fv(std::int32_t location, std::int32_t count, bool transpose, const float * value) noexcept { return mygl::get_static_dispatch().glUniformMatrix3x2fv(location, count, transpose, value); }
void glUniformMatrix3x4dv(std::int32_t location, std::int32_t count, bool transpose, const double * value) noexcept { return mygl::get_static_dispatch().glUniformMatrix3x4dv(location, count, transpose, value); }
void glUniformMatrix3x4fv(std::int32_t location, std::int32_t count, bool transpose, const float * value) noexcept { return mygl::get_static_dispatch().glUniformMatrix3x4fv(location, count, transpose, value); }
void glUniformMatrix4dv(std::int32_t location, std::int32_t count, bool transpose, const double * value) noexcept { return mygl::get_static_dispatch().glUniformMatrix4dv(location, count, transpose, value); }
void glUniformMatrix4fv(std::int32_t location, std::int32_t count, bool transpose, const float * value) noexcept { return mygl::get_static_dispatch().glUniformMatrix4fv(location, count, transpose, value); }
void glUniformMatrix4x2dv(std::int32_t location, std::int32_t count, bool transpose, const double * value) noexcept { return mygl::get_static_dispatch().glUniformMatrix4x2dv(location, count, transpose, value); }
void glUniformMatrix4x2fv(std::int32_t location, std::int32_t count, bool transpose, const float * value) noexcept { return mygl::get_static_dispatch().glUniformMatrix4x2fv(location, count, transpose, value); }
void glUniformMatrix4x3dv(std::int32_t location, std::int32_t count, bool transpose, const double * value) noexcept { return mygl::get_static_dispatch().glUniformMatrix4x3dv(location, count, transpose, value); }
void glUniformMatrix4x3fv(std::int32_t location, std::int32_t count, bool transpose, const float * value) noexcept { return mygl::get_static_dispatch().glUniformMatrix4x3fv(location, count, transpose, value); }
void glUniformSubroutinesuiv(GLenum shadertype, std::int32_t count, const std::uint32_t * indices) noexcept { return mygl::get_static_dispatch().glUniformSubroutinesuiv(shadertype, count, indices); }
void glUniformui64NV(std::int32_t location, std::uint64_t value) noexcept { return mygl::get_static_dispatch().glUniformui64NV(location, value); }
void glUniformui64vNV(std::int32_t location, std::int32_t count, const std::uint64_t * value) noexcept { return mygl::get_static_dispatch().glUniformui64vNV(location, count, value); }
bool glUnmapBuffer(GLenum target) noexcept { return mygl::get_static_dispatch().glUnmapBuffer(target); }
bool glUnmapNamedBuffer(std::uint32_t buffer) noexcept { return mygl::get_static_dispatch().glUnmapNamedBuffer(buffer); }
void glUseProgram(std::uint32_t program) noexcept { return mygl::get_static_dispatch().glUseProgram(program); }
void glUseProgramStages(std::uint32_t pipeline, GLbitfield stages, std::uint32_t program) noexcept { return mygl::get_static_dispatch().glUseProgramStages(pipeline, stages, program); }
void glValidateProgram(std::uint32_t program) noexcept { return mygl::get_static_dispatch().glValidateProgram(program); }
void glValidateProgramPipeline(std::uint32_t pipeline) noexcept { return mygl::get_static_dispatch().glValidateProgramPipeline(pipeline); }
void glVertexArrayAttribBinding(std::uint32_t vaobj, std::uint32_t attribindex, std::uint32_t bindingindex) noexcept { return mygl::get_static_dispatch().glVertexArrayAttribBinding(vaobj, attribindex, bindingindex); }
void glVertexArrayAttribFormat(std::uint32_t vaobj, std::uint32_t attribindex, std::int32_t size, GLenum type, bool normalized, std::uint32_t relativeoffset) noexcept { return mygl::get_static_dispatch().glVertexArrayAttribFormat(vaobj, attribindex, size, type, normalized, relativeoffset); }
void glVertexArrayAttribIFormat(std::uint32_t vaobj, std::uint32_t attribindex, std::int32_t size, GLenum type, std::uint32_t relativeoffset) noexcept { return mygl::get_static_dispatch().glVertexArrayAttribIFormat(vaobj, attribindex, size, type, relativeoffset); }
void glVertexArrayAttribLFormat(std::uint32_t vaobj, std::uint32_t attribindex, std::int32_t size, GLenum type, std::uint32_t relativeoffset) noexcept { return mygl::get_static_dispatch().glVertexArrayAttribLFormat(vaobj, attribindex, size, type, relativeoffset); }
void glVertexArrayBindingDivisor(std::uint32_t vaobj, std::uint32_t bindingindex, std::uint32_t divisor) noexcept { return mygl::get_static_dispatch().glVertexArrayBindingDivisor(vaobj, bindingindex, divisor); }
void glVertexArrayElementBuffer(std::uint32_t vaobj, std::uint32_t buffer) noexcept { return mygl::get_static_dispatch().glVertexArrayElementBuffer(vaobj, buffer); }
void glVertexArrayVertexBuffer(std::uint32_t vaobj, std::uint32_t bindingindex, std::uint32_t buffer, std::intptr_t offset, std::int32_t stride) noexcept { return mygl::get_static_dispatch().glVertexArrayVertexBuffer(vaobj, bindingindex, buffer, offset, stride); }
void glVertexArrayVertexBuffers(std::uint32_t vaobj, std::uint32_t first, std::int32_t count, const std::uint32_t * buffers, const std::intptr_t * offsets, const std::int32_t * strides) noexcept { return mygl::get_static_dispatch().glVertexArrayVertexBuffers(vaobj, first, count, buffers, offsets, strides); }
void glVertexAttrib1d(std::uint32_t index, double x) noexcept { return mygl::get_static_dispatch().glVertexAttrib1d(index, x); }
void glVertexAttrib1dv(std::uint32_t index, const double * v) noexcept { return mygl::get_static_dispatch().glVertexAttrib1dv(index, v); }
void glVertexAttrib1f(std::uint32_t index, float x) noexcept { return mygl::get_static_dispatch().glVertexAttrib1f(index, x); }
void glVertexAttrib1fv(std::uint32_t index, const float * v) noexcept { return mygl::get_static_dispatch().glVertexAttrib1fv(index, v); }
void glVertexAttrib1s(std::uint32_t index, std::int16_t x) noexcept { return mygl::get_static_dispatch().glVertexAttrib1s(index, x); }
void glVertexAttrib1sv(std::uint32_t index, const std::int16_t * v) noexcept { return mygl::get_static_dispatch().glVertexAttrib1sv(index, v); }
void glVertexAttrib2d(std::uint32_t index, double x, double y) noexcept { return mygl::get_static_dispatch().glVertexAttrib2d(index, x, y); }
void glVertexAttrib2dv(std::uint32_t index, const double * v) noexcept { return mygl::get_static_dispatch().glVertexAttrib2dv(index, v); }
void glVertexAttrib2f(std::uint32_t index, float x, float y) noexcept { return mygl::get_static_dispatch().glVertexAttrib2f(index, x, y); }
void glVertexAttrib2fv(std::uint32_t index, const float * v) noexcept { return mygl::get_static_dispatch().glVertexAttrib2fv(index, v); }
void glVertexAttrib2s(std::uint32_t index, std::int16_t x, std::int16_t y) noexcept { return mygl::get_static_dispatch().glVertexAttrib2s(index, x, y); }
void glVertexAttrib2sv(std::uint32_t index, const std::int16_t * v) noexcept { return mygl::get_static_dispatch().glVertexAttrib2sv(index, v); }
void glVertexAttrib3d(std::uint32_t index, double x, double y, double z) noexcept { return mygl::get_static_dispatch().glVertexAttrib3d(index, x, y, z); }
void glVertexAttrib3dv(std::uint32_t index, const double * v) noexcept { return mygl::get_static_dispatch().glVertexAttrib3dv(index, v); }
void glVertexAttrib3f(std::uint32_t index, float x, float y, float z) noexcept { return mygl::get_static_dispatch().glVertexAttrib3f(index, x, y, z); }
void glVertexAttrib3fv(std::uint32_t index, const float * v) noexcept { return mygl::get_static_dispatch().glVertexAttrib3fv(index, v); }
void glVertexAttrib3s(std::uint32_t index, std::int16_t x, std::int16_t y, std::int16_t z) noexcept { return mygl::get_static_dispatch().glVertexAttrib3s(index, x, y, z); }
void glVertexAttrib3sv(std::uint32_t index, const std::int16_t * v) noexcept { return mygl::get_static_dispatch().glVertexAttrib3sv(index, v); }
void glVertexAttrib4Nbv(std::uint32_t index, const std::int8_t * v) noexcept { return mygl::get_static_dispatch().glVertexAttrib4Nbv(index, v); }
void glVertexAttrib4Niv(std::uint32_t index, const std::int32_t * v) noexcept { return mygl::get_static_dispatch().glVertexAttrib4Niv(index, v); }
void glVertexAttrib4Nsv(std::uint32_t index, const std::int16_t * v) noexcept { return mygl::get_static_dispatch().glVertexAttrib4Nsv(index, v); }
void glVertexAttrib4Nub(std::uint32_t index, std::uint8_t x, std::uint8_t y, std::uint8_t z, std::uint8_t w) noexcept { return mygl::get_static_dispatch().glVertexAttrib4Nub(index, x, y, z, w); }
void glVertexAttrib4Nubv(std::uint32_t index, const std::uint8_t * v) noexcept { return mygl::get_static_dispatch().glVertexAttrib4Nubv(index, v); }
void glVertexAttrib4Nuiv(std::uint32_t index, const std::uint32_t * v) noexcept { return mygl::get_static_dispatch().glVertexAttrib4Nuiv(index, v); }
void glVertexAttrib4Nusv(std::uint32_t index, const std::uint16_t * v) noexcept { return mygl::get_static_dispatch().glVertexAttrib4Nusv(index, v); }
void glVertexAttrib4bv(std::uint32_t index, const std::int8_t * v) noexcept { return mygl::get_static_dispatch().glVertexAttrib4bv(index, v); }
void glVertexAttrib4d(std::uint32_t index, double x, double y, double z, double w) noexcept { return mygl::get_static_dispatch().glVertexAttrib4d(index, x, y, z, w); }
void glVertexAttrib4dv(std::uint32_t index, const double * v) noexcept { return mygl::get_static_dispatch().glVertexAttrib4dv(index, v); }
void glVertexAttrib4f(std::uint32_t index, float x, float y, float z, float w) noexcept { return mygl::get_static_dispatch().glVertexAttrib4f(index, x, y, z, w); }
void glVertexAttrib4fv(std::uint32_t index, const float * v) noexcept { return mygl::get_static_dispatch().glVertexAttrib4fv(index, v); }
void glVertexAttrib4iv(std::uint32_t index, const std::int32_t * v) noexcept { return mygl::get_static_dispatch().glVertexAttrib4iv(index, v); }
void glVertexAttrib4s(std::uint32_t index, std::int16_t x, std::int16_t y, std::int16_t z, std::int16_t w) noexcept { return mygl::get_static_dispatch().glVertexAttrib4s(index, x, y, z, w); }
void glVertexAttrib4sv(std::uint32_t index, const std::int16_t * v) noexcept { return mygl::get_static_dispatch().glVertexAttrib4sv(index, v); }
void glVertexAttrib4ubv(std::uint32_t index, const std::uint8_t * v) noexcept { return mygl::get_static_dispatch().glVertexAttrib4ubv(index, v); }
void glVertexAttrib4uiv(std::uint32_t index, const std::uint32_t * v) noexcept { return mygl::get_static_dispatch().glVertexAttrib4uiv(index, v); }
void glVertexAttrib4usv(std::uint32_t index, const std::uint16_t * v) noexcept { return mygl::get_static_dispatch().glVertexAttrib4usv(index, v); }
void glVertexAttribBinding(std::uint32_t attribindex, std::uint32_t bindingindex) noexcept { return mygl::get_static_dispatch().glVertexAttribBinding(attribindex, bindingindex); }
void glVertexAttribDivisor(std::uint32_t index, std::uint32_t divisor) noexcept { return mygl::get_static_dispatch().glVertexAttribDivisor(index, divisor); }
void glVertexAttribFormat(std::uint32_t attribindex, std::int32_t size, GLenum type, bool normalized, std::uint32_t relativeoffset) noexcept { return mygl::get_static_dispatch().glVertexAttribFormat(attribindex, size, type, normalized, relativeoffset); }
void glVertexAttribFormatNV(std::uint32_t index, std::int32_t size, GLenum type, bool normalized, std::int32_t stride) noexcept { return mygl::get_static_dispatch().glVertexAttribFormatNV(index, size, type, normalized, stride); }
void glVertexAttribI1i(std::uint32_t index, std::int32_t x) noexcept { return mygl::get_static_dispatch().glVertexAttribI1i(index, x); }
void glVertexAttribI1iv(std::uint32_t index, const std::int32_t * v) noexcept { return mygl::get_static_dispatch().glVertexAttribI1iv(index, v); }
void glVertexAttribI1ui(std::uint32_t index, std::uint32_t x) noexcept { return mygl::get_static_dispatch().glVertexAttribI1ui(index, x); }
void glVertexAttribI1uiv(std::uint32_t index, const std::uint32_t * v) noexcept { return mygl::get_static_dispatch().glVertexAttribI1uiv(index, v); }
void glVertexAttribI2i(std::uint32_t index, std::int32_t x, std::int32_t y) noexcept { return mygl::get_static_dispatch().glVertexAttribI2i(index, x, y); }
void glVertexAttribI2iv(std::uint32_t index, const std::int32_t * v) noexcept { return mygl::get_static_dispatch().glVertexAttribI2iv(index, v); }
void glVertexAttribI2ui(std::uint32_t index, std::uint32_t x, std::uint32_t y) noexcept { return mygl::get_static_dispatch().glVertexAttribI2ui(index, x, y); }
void glVertexAttribI2uiv(std::uint32_t index, const std::uint32_t * v) noexcept { return mygl::get_static_dispatch().glVertexAttribI2uiv(index, v); }
void glVertexAttribI3i(std::uint32_t index, std::int32_t x, std::int32_t y, std::int32_t z) noexcept { return mygl::get_static_dispatch().glVertexAttribI3i(index, x, y, z); }
void glVertexAttribI3iv(std::uint32_t index, const std::int32_t * v) noexcept { return mygl::get_static_dispatch().glVertexAttribI3iv(index, v); }
void glVertexAttribI3ui(std::uint32_t index, std::uint32_t x, std::uint32_t y, std::uint32_t z) noexcept { return mygl::get_static_dispatch().glVertexAttribI3ui(index, x, y, z); }
void glVertexAttribI3uiv(std::uint32_t index, const std::uint32_t * v) noexcept { return mygl::get_static_dispatch().glVertexAttribI3uiv(index, v); }
void glVertexAttribI4bv(std::uint32_t index, const std::int8_t * v) noexcept { return mygl::get_static_dispatch().glVertexAttribI4bv(index, v); }
void glVertexAttribI4i(std::uint32_t index, std::int32_t x, std::int32_t y, std::int32_t z, std::int32_t w) noexcept { return mygl::get_static_dispatch().glVertexAttribI4i(index, x, y, z, w); }
void glVertexAttribI4iv(std::uint32_t index, const std::int32_t * v) noexcept { return mygl::get_static_dispatch().glVertexAttribI4iv(index, v); }
void glVertexAttribI4sv(std::uint32_t index, const std::int16_t * v) noexcept { return mygl::get_static_dispatch().glVertexAttribI4sv(index, v); }
void glVertexAttribI4ubv(std::uint32_t index, const std::uint8_t * v) noexcept { return mygl::get_static_dispatch().glVertexAttribI4ubv(index, v); }
void glVertexAttribI4ui(std::uint32_t index, std::uint32_t x, std::uint32_t y, std::uint32_t z, std::uint32_t w) noexcept { return mygl::get_static_dispatch().glVertexAttribI4ui(index, x, y, z, w); }
void glVertexAttribI4uiv(std::uint32_t index, const std::uint32_t * v) noexcept { return mygl::get_static_dispatch().glVertexAttribI4uiv(index, v); }
void glVertexAttribI4usv(std::uint32_t index, const std::uint16_t * v) noexcept { return mygl::get_static_dispatch().glVertexAttribI4usv(index, v); }
void glVertexAttribIFormat(std::uint32_t attribindex, std::int32_t size, GLenum type, std::uint32_t relativeoffset) noexcept { return mygl::get_static_dispatch().glVertexAttribIFormat(attribindex, size, type, relativeoffset); }
void glVertexAttribIFormatNV(std::uint32_t index, std::int32_t size, GLenum type, std::int32_t stride) noexcept { return mygl::get_static_dispatch().glVertexAttribIFormatNV(index, size, type, stride); }
void glVertexAttribIPointer(std::uint32_t index, std::int32_t size, GLenum type, std::int32_t stride, const void * pointer) noexcept { return mygl::get_static_dispatch().glVertexAttribIPointer(index, size, type, stride, pointer); }
void glVertexAttribL1d(std::uint32_t index, double x) noexcept { return mygl::get_static_dispatch().glVertexAttribL1d(index, x); }
void glVertexAttribL1dv(std::uint32_t index, const double * v) noexcept { return mygl::get_static_dispatch().glVertexAttribL1dv(index, v); }
void glVertexAttribL1ui64ARB(std::uint32_t index, std::uint64_t x) noexcept { return mygl::get_static_dispatch().glVertexAttribL1ui64ARB(index, x); }
void glVertexAttribL1ui64vARB(std::uint32_t index, const std::uint64_t * v) noexcept { return mygl::get_static_dispatch().glVertexAttribL1ui64vARB(index, v); }
void glVertexAttribL2d(std::uint32_t index, double x, double y) noexcept { return mygl::get_static_dispatch().glVertexAttribL2d(index, x, y); }
void glVertexAttribL2dv(std::uint32_t index, const double * v) noexcept { return mygl::get_static_dispatch().glVertexAttribL2dv(index, v); }
void glVertexAttribL3d(std::uint32_t index, double x, double y, double z) noexcept { return mygl::get_static_dispatch().glVertexAttribL3d(index, x, y, z); }
void glVertexAttribL3dv(std::uint32_t index, const double * v) noexcept { return mygl::get_static_dispatch().glVertexAttribL3dv(index, v); }
void glVertexAttribL4d(std::uint32_t index, double x, double y, double z, double w) noexcept { return mygl::get_static_dispatch().glVertexAttribL4d(index, x, y, z, w); }
void glVertexAttribL4dv(std::uint32_t index, const double * v) noexcept { return mygl::get_static_dispatch().glVertexAttribL4dv(index, v); }
void glVertexAttribLFormat(std::uint32_t attribindex, std::int32_t size, GLenum type, std::uint32_t relativeoffset) noexcept { return mygl::get_static_dispatch().glVertexAttribLFormat(attribindex, size, type, relativeoffset); }
void glVertexAttribLPointer(std::uint32_t index, std::int32_t size, GLenum type, std::int32_t stride, const void * pointer) noexcept { return mygl::get_static_dispatch().glVertexAttribLPointer(index, size, type, stride, pointer); }
void glVertexAttribP1ui(std::uint32_t index, GLenum type, bool normalized, std::uint32_t value) noexcept { return mygl::get_static_dispatch().glVertexAttribP1ui(index, type, normalized, value); }
void glVertexAttribP1uiv(std::uint32_t index, GLenum type, bool normalized, const std::uint32_t * value) noexcept { return mygl::get_static_dispatch().glVertexAttribP1uiv(index, type, normalized, value); }
void glVertexAttribP2ui(std::uint32_t index, GLenum type, bool normalized, std::uint32_t value) noexcept { return mygl::get_static_dispatch().glVertexAttribP2ui(index, type, normalized, value); }
void glVertexAttribP2uiv(std::uint32_t index, GLenum type, bool normalized, const std::uint32_t * value) noexcept { return mygl::get_static_dispatch().glVertexAttribP2uiv(index, type, normalized, value); }
void glVertexAttribP3ui(std::uint32_t index, GLenum type, bool normalized, std::uint32_t value) noexcept { return mygl::get_static_dispatch().glVertexAttribP3ui(index, type, normalized, value); }
void glVertexAttribP3uiv(std::uint32_t index, GLenum type, bool normalized, const std::uint32_t * value) noexcept { return mygl::get_static_dispatch().glVertexAttribP3uiv(index, type, normalized, value); }
void glVertexAttribP4ui(std::uint32_t index, GLenum type, bool normalized, std::uint32_t value) noexcept { return mygl::get_static_dispatch().glVertexAttribP4ui(index, type, normalized, value); }
void glVertexAttribP4uiv(std::uint32_t index, GLenum type, bool normalized, const std::uint32_t * value) noexcept { return mygl::get_static_dispatch().glVertexAttribP4uiv(index, type, normalized, value); }
void glVertexAttribPointer(std::uint32_t index, std::int32_t size, GLenum type, bool normalized, std::int32_t stride, const void * pointer) noexcept { return mygl::get_static_dispatch().glVertexAttribPointer(index, size, type, normalized, stride, pointer); }
void glVertexBindingDivisor(std::uint32_t bindingindex, std::uint32_t divisor) noexcept { return mygl::get_static_dispatch().glVertexBindingDivisor(bindingindex, divisor); }
void glVertexFormatNV(std::int32_t size, GLenum type, std::int32_t stride) noexcept { return mygl::get_static_dispatch().glVertexFormatNV(size, type, stride); }
void glVertexP2ui(GLenum type, std::uint32_t value) noexcept { return mygl::get_static_dispatch().glVertexP2ui(type, value); }
void glVertexP2uiv(GLenum type, const std::uint32_t * value) noexcept { return mygl::get_static_dispatch().glVertexP2uiv(type, value); }
void glVertexP3ui(GLenum type, std::uint32_t value) noexcept { return mygl::get_static_dispatch().glVertexP3ui(type, value); }
void glVertexP3uiv(GLenum type, const std::uint32_t * value) noexcept { return mygl::get_static_dispatch().glVertexP3uiv(type, value); }
void glVertexP4ui(GLenum type, std::uint32_t value) noexcept { return mygl::get_static_dispatch().glVertexP4ui(type, value); }
void glVertexP4uiv(GLenum type, const std::uint32_t * value) noexcept { return mygl::get_static_dispatch().glVertexP4uiv(type, value); }
void glViewport(std::int32_t x, std::int32_t y, std::int32_t width, std::int32_t height) noexcept { return mygl::get_static_dispatch().glViewport(x, y, width, height); }
void glViewportArrayv(std::uint32_t first, std::int32_t count, const float * v) noexcept { return mygl::get_static_dispatch().glViewportArrayv(first, count, v); }
void glViewportIndexedf(std::uint32_t index, float x, float y, float w, float h) noexcept { return mygl::get_static_dispatch().glViewportIndexedf(index, x, y, w, h); }
void glViewportIndexedfv(std::uint32_t index, const float * v) noexcept { return mygl::get_static_dispatch().glViewportIndexedfv(index, v); }
void glWaitSemaphoreEXT(std::uint32_t semaphore, std::uint32_t numBufferBarriers, const std::uint32_t * buffers, std::uint32_t numTextureBarriers, const std::uint32_t * textures, const GLenum * srcLayouts) noexcept { return mygl::get_static_dispatch().glWaitSemaphoreEXT(semaphore, numBufferBarriers, buffers, numTextureBarriers, textures, srcLayouts); }
void glWaitSync(struct __GLsync * sync, GLbitfield flags, std::uint64_t timeout) noexcept { return mygl::get_static_dispatch().glWaitSync(sync, flags, timeout); }
void glWeightPathsNV(std::uint32_t resultPath, std::int32_t numPaths, const std::uint32_t * paths, const float * weights) noexcept { return mygl::get_static_dispatch().glWeightPathsNV(resultPath, numPaths, paths, weights); }
