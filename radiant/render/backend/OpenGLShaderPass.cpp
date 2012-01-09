#include "OpenGLShaderPass.h"
#include "OpenGLShader.h"

#include "math/Matrix4.h"
#include "math/AABB.h"
#include "irender.h"
#include "ishaders.h"
#include "texturelib.h"
#include "iglprogram.h"

namespace {

// Bind the given texture to the texture unit, if it is different from the
// current state, then set the current state to the new texture.
inline void setTextureState(GLint& current,
							const GLint& texture,
							GLenum textureUnit,
                            GLenum textureMode)
{
    if (texture != current)
    {
        glActiveTexture(textureUnit);
        glClientActiveTexture(textureUnit);
        glBindTexture(textureMode, texture);
        GlobalOpenGL().assertNoErrors();
        current = texture;
    }
}

// Same as setTextureState() above without texture unit parameter
inline void setTextureState(GLint& current,
                            const GLint& texture,
                            GLenum textureMode)
{
	if (texture != current)
	{
		glBindTexture(textureMode, texture);
		GlobalOpenGL().assertNoErrors();
		current = texture;
	}
}

// Utility function to toggle an OpenGL state flag
inline void setState(unsigned int state,
					 unsigned int delta,
					 unsigned int flag,
					 GLenum glflag)
{
	if (delta & state & flag)
	{
		glEnable(glflag);
		GlobalOpenGL().assertNoErrors();
	}
	else if(delta & ~state & flag)
	{
		glDisable(glflag);
		GlobalOpenGL().assertNoErrors();
	}
}

inline void evaluateStage(const ShaderLayerPtr& stage, std::size_t time, const IRenderEntity* entity)
{
	if (stage) 
	{
		if (entity)
		{
			stage->evaluateExpressions(time, *entity);
		}
		else
		{
			stage->evaluateExpressions(time);
		}
	}
}

} // namespace

// GL state enabling/disabling helpers

void OpenGLShaderPass::setTexture0()
{
    if (GLEW_VERSION_1_3)
    {
        glActiveTexture(GL_TEXTURE0);
        glClientActiveTexture(GL_TEXTURE0);
    }
}

void OpenGLShaderPass::enableTexture2D()
{
    GlobalOpenGL().assertNoErrors();

    setTexture0();
    glEnable(GL_TEXTURE_2D);
	
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    GlobalOpenGL().assertNoErrors();
}

void OpenGLShaderPass::disableTexture2D()
{
    setTexture0();
    glDisable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);

    GlobalOpenGL().assertNoErrors();
}

// Enable cubemap texturing and texcoord array
void OpenGLShaderPass::enableTextureCubeMap()
{
    setTexture0();
    glEnable(GL_TEXTURE_CUBE_MAP);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    GlobalOpenGL().assertNoErrors();
}

// Disable cubemap texturing and texcoord array
void OpenGLShaderPass::disableTextureCubeMap()
{
    setTexture0();
    glDisable(GL_TEXTURE_CUBE_MAP);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);

    GlobalOpenGL().assertNoErrors();
}

void OpenGLShaderPass::enableRenderBlend()
{
    glEnable(GL_BLEND);
    setTexture0();
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    GlobalOpenGL().assertNoErrors();
}

void OpenGLShaderPass::disableRenderBlend()
{
    glDisable(GL_BLEND);
    setTexture0();
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    GlobalOpenGL().assertNoErrors();
}

void OpenGLShaderPass::setupTextureMatrix(GLenum textureUnit, const ShaderLayerPtr& stage)
{
	// Set the texture matrix for the given unit 
	glActiveTexture(textureUnit);
	glClientActiveTexture(textureUnit);

	if (stage)
	{
		static const Matrix4 transMinusHalf = Matrix4::getTranslation(Vector3(-0.5f, -0.5f, 0));
		static const Matrix4 transPlusHalf = Matrix4::getTranslation(Vector3(+0.5f, +0.5f, 0));

		Matrix4 tex = Matrix4::getIdentity();

		Vector2 scale = stage->getScale();

		if (stage->getStageFlags() & ShaderLayer::FLAG_CENTERSCALE)
		{
			// Center scale, apply translation by -0.5 first, then scale, then translate back
			tex.multiplyBy(transMinusHalf);
			tex.multiplyBy(Matrix4::getScale(Vector3(scale.x(), scale.y(), 1)));
			tex.multiplyBy(transPlusHalf);
		}
		else
		{
			// Regular scale, apply translation and scale
			tex.multiplyBy(Matrix4::getScale(Vector3(scale.x(), scale.y(), 1)));
		}

		Vector2 shear = stage->getShear();

		if (shear.x() != 0 || shear.y() != 0)
		{
			Matrix4 shearMatrix = Matrix4::byColumns(
				1, shear.y(), 0, 0,
				shear.x(), 1, 0, 0,
				0, 0, 1, 0,
				0, 0, 0, 1
			);

			tex.multiplyBy(transMinusHalf);
			tex.multiplyBy(shearMatrix);
			tex.multiplyBy(transPlusHalf);
		}
		
		// Rotation
		float rotate = stage->getRotation();

		if (rotate != 0)
		{
			float angle = rotate * 2 * static_cast<float>(c_pi);
		
			Matrix4 rot = Matrix4::getRotationAboutZ(angle);	

			tex.multiplyBy(transMinusHalf);
			tex.multiplyBy(rot);
			tex.multiplyBy(transPlusHalf);
		}

		// Apply translation as last step
		Vector2 translation = stage->getTranslation();
		tex.multiplyBy(Matrix4::getTranslation(Vector3(translation.x(), translation.y(), 0)));

		glLoadMatrixf(tex);
	}
	else
	{
		glLoadMatrixf(Matrix4::getIdentity());
	}
}

// Apply all textures to texture units
void OpenGLShaderPass::applyAllTextures(OpenGLState& current,
                                        unsigned requiredState)
{
    // Set the texture dimensionality from render flags. There is only a global
    // mode for all textures, we can't have texture1 as 2D and texture2 as
    // CUBE_MAP for example.
    GLenum textureMode = 0;

    if (requiredState & RENDER_TEXTURE_CUBEMAP) // cube map has priority
	{
        textureMode = GL_TEXTURE_CUBE_MAP;
	}
    else if (requiredState & RENDER_TEXTURE_2D)
	{
        textureMode = GL_TEXTURE_2D;
	}

    // Apply our texture numbers to the current state
    if (textureMode != 0) // only if one of the RENDER_TEXTURE options
    {
		glMatrixMode(GL_TEXTURE);

        if (GLEW_VERSION_1_3)
        {
			setTextureState(current.texture0, _state.texture0, GL_TEXTURE0, textureMode);
			setupTextureMatrix(GL_TEXTURE0, _state.stage0);

			setTextureState(current.texture1, _state.texture1, GL_TEXTURE1, textureMode);
			setupTextureMatrix(GL_TEXTURE1, _state.stage1);

            setTextureState(current.texture2, _state.texture2, GL_TEXTURE2, textureMode);
			setupTextureMatrix(GL_TEXTURE2, _state.stage2);

            setTextureState(current.texture3, _state.texture2, GL_TEXTURE2, textureMode);
			setTextureState(current.texture4, _state.texture2, GL_TEXTURE2, textureMode);

			glActiveTexture(GL_TEXTURE0);
			glClientActiveTexture(GL_TEXTURE0);
        }
        else
        {
            setTextureState(current.texture0, _state.texture0, textureMode);
			setupTextureMatrix(GL_TEXTURE0, _state.stage0);
        }

		glMatrixMode(GL_MODELVIEW);
    }
}

// Set up cube map
void OpenGLShaderPass::setUpCubeMapAndTexGen(OpenGLState& current,
                                             unsigned requiredState,
                                             const Vector3& viewer)
{
    if (requiredState & RENDER_TEXTURE_CUBEMAP)
    {
        // Copy cubemap mode enum to current state object
        current.cubeMapMode = _state.cubeMapMode;

        // Apply axis transformation (swap Y and Z coordinates)
        Matrix4 transform = Matrix4::byRows(
            1, 0, 0, 0,
            0, 0, 1, 0,
            0, 1, 0, 0,
            0, 0, 0, 1
        );

        // Subtract the viewer position
        transform.translateBy(-viewer);

        // Apply to the texture matrix
        glMatrixMode(GL_TEXTURE);
        glLoadMatrixf(transform);
        glMatrixMode(GL_MODELVIEW);
    }
}

// Apply own state to current state object
void OpenGLShaderPass::applyState(OpenGLState& current,
					              unsigned int globalStateMask,
                                  const Vector3& viewer,
								  std::size_t time,
								  const IRenderEntity* entity)
{
	// Evaluate any shader expressions
	if (_state.stage0) 
	{
		evaluateStage(_state.stage0, time, entity);

		// The alpha test value might change over time
		if (_state.stage0->getAlphaTest() > 0)
		{
			_state.renderFlags |= RENDER_ALPHATEST;
		}
		else
		{
			_state.renderFlags &= ~RENDER_ALPHATEST;
		}
	}

	if (_state.stage1) evaluateStage(_state.stage1, time, entity);
	if (_state.stage2) evaluateStage(_state.stage2, time, entity);
	if (_state.stage3) evaluateStage(_state.stage3, time, entity);
	if (_state.stage4) evaluateStage(_state.stage4, time, entity);

	if (_state.renderFlags & RENDER_OVERRIDE)
	{
		globalStateMask |= RENDER_FILL | RENDER_DEPTHWRITE;
	}

	// Apply the global state mask to our own desired render flags to determine
    // the final set of flags that must bet set
	unsigned int requiredState = _state.renderFlags & globalStateMask;

	// In per-entity mode, allow the entity to add requirements
	if (entity != NULL)
	{
		requiredState |= entity->getRequiredShaderFlags();
	}

    // Construct a mask containing all the flags that will be changing between
    // the current state and the required state. This avoids performing
    // unnecessary GL calls to set the state to its existing value.
	const unsigned int changingBitsMask = requiredState ^ current.renderFlags;

    // Set the GLProgram if required
	GLProgram* program = (requiredState & RENDER_PROGRAM) != 0
						  ? _state.glProgram
						  : 0;

    if (program != current.glProgram)
    {
        if (current.glProgram != 0)
        {
			current.glProgram->disable();
			glColor4fv(current.m_colour);
        }

        current.glProgram = program;

        if (current.glProgram != 0)
        {
			current.glProgram->enable();
        }
    }

    // State changes. Only perform these if changingBitsMask > 0, since if there are
    // no changes required we don't want a whole load of unnecessary bit
    // operations.
    if (changingBitsMask != 0)
    {
        if(changingBitsMask & requiredState & RENDER_FILL)
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            GlobalOpenGL().assertNoErrors();
        }
        else if(changingBitsMask & ~requiredState & RENDER_FILL)
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            GlobalOpenGL().assertNoErrors();
        }

        setState(requiredState, changingBitsMask, RENDER_OFFSETLINE, GL_POLYGON_OFFSET_LINE);

        if(changingBitsMask & requiredState & RENDER_LIGHTING)
        {
            glEnable(GL_LIGHTING);
            glEnable(GL_COLOR_MATERIAL);
            glEnableClientState(GL_NORMAL_ARRAY);
            GlobalOpenGL().assertNoErrors();
        }
        else if(changingBitsMask & ~requiredState & RENDER_LIGHTING)
        {
            glDisable(GL_LIGHTING);
            glDisable(GL_COLOR_MATERIAL);
            glDisableClientState(GL_NORMAL_ARRAY);
            GlobalOpenGL().assertNoErrors();
        }

        // RENDER_TEXTURE_CUBEMAP
        if(changingBitsMask & requiredState & RENDER_TEXTURE_CUBEMAP)
        {
            enableTextureCubeMap();
        }
        else if(changingBitsMask & ~requiredState & RENDER_TEXTURE_CUBEMAP)
        {
            disableTextureCubeMap();
        }

		// RENDER_TEXTURE_2D
        if(changingBitsMask & requiredState & RENDER_TEXTURE_2D)
        {
            enableTexture2D();
        }
        else if(changingBitsMask & ~requiredState & RENDER_TEXTURE_2D)
        {
            disableTexture2D();
        }

        // RENDER_BLEND
        if(changingBitsMask & requiredState & RENDER_BLEND)
        {
            enableRenderBlend();
        }
        else if(changingBitsMask & ~requiredState & RENDER_BLEND)
        {
            disableRenderBlend();
        }

        setState(requiredState, changingBitsMask, RENDER_CULLFACE, GL_CULL_FACE);

        if(changingBitsMask & requiredState & RENDER_SMOOTH)
        {
            glShadeModel(GL_SMOOTH);
            GlobalOpenGL().assertNoErrors();
        }
        else if(changingBitsMask & ~requiredState & RENDER_SMOOTH)
        {
            glShadeModel(GL_FLAT);
            GlobalOpenGL().assertNoErrors();
        }

        setState(requiredState, changingBitsMask, RENDER_SCALED, GL_NORMALIZE); // not GL_RESCALE_NORMAL

        setState(requiredState, changingBitsMask, RENDER_DEPTHTEST, GL_DEPTH_TEST);

        if(changingBitsMask & requiredState & RENDER_DEPTHWRITE)
        {
            glDepthMask(GL_TRUE);

            GlobalOpenGL().assertNoErrors();
        }
        else if(changingBitsMask & ~requiredState & RENDER_DEPTHWRITE)
        {
            glDepthMask(GL_FALSE);

            GlobalOpenGL().assertNoErrors();
        }

        if(changingBitsMask & requiredState & RENDER_COLOURWRITE)
        {
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            GlobalOpenGL().assertNoErrors();
        }
        else if(changingBitsMask & ~requiredState & RENDER_COLOURWRITE)
        {
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
            GlobalOpenGL().assertNoErrors();
        }

        setState(requiredState, changingBitsMask, RENDER_ALPHATEST, GL_ALPHA_TEST);

        if ((changingBitsMask & requiredState & RENDER_COLOURARRAY))
        {
            glEnableClientState(GL_COLOR_ARRAY);
            GlobalOpenGL().assertNoErrors();
        }
        else if(changingBitsMask & ~requiredState & RENDER_COLOURARRAY)
        {
            glDisableClientState(GL_COLOR_ARRAY);
            glColor4fv(_state.m_colour);
            GlobalOpenGL().assertNoErrors();
        }

        if(changingBitsMask & ~requiredState & RENDER_COLOURCHANGE)
        {
            glColor4fv(_state.m_colour);
            GlobalOpenGL().assertNoErrors();
        }

        // Set GL states corresponding to RENDER_ flags
        setState(requiredState, changingBitsMask, RENDER_LINESTIPPLE, GL_LINE_STIPPLE);
        setState(requiredState, changingBitsMask, RENDER_LINESMOOTH, GL_LINE_SMOOTH);

        setState(requiredState, changingBitsMask, RENDER_POLYGONSTIPPLE, GL_POLYGON_STIPPLE);
        setState(requiredState, changingBitsMask, RENDER_POLYGONSMOOTH, GL_POLYGON_SMOOTH);

    } // end of changingBitsMask-dependent changes

  if(requiredState & RENDER_DEPTHTEST && _state.m_depthfunc != current.m_depthfunc)
  {
    glDepthFunc(_state.m_depthfunc);
    GlobalOpenGL().assertNoErrors();
    current.m_depthfunc = _state.m_depthfunc;
  }

  if(requiredState & RENDER_LINESTIPPLE
    && (_state.m_linestipple_factor != current.m_linestipple_factor
    || _state.m_linestipple_pattern != current.m_linestipple_pattern))
  {
    glLineStipple(_state.m_linestipple_factor, _state.m_linestipple_pattern);
    GlobalOpenGL().assertNoErrors();
    current.m_linestipple_factor = _state.m_linestipple_factor;
    current.m_linestipple_pattern = _state.m_linestipple_pattern;
  }

    // Set up the alpha test parameters
    if (requiredState & RENDER_ALPHATEST
        && ( _state.alphaFunc != current.alphaFunc
            || _state.alphaThreshold != current.alphaThreshold)
    )
    {
        // Set alpha function in GL
        glAlphaFunc(_state.alphaFunc, _state.alphaThreshold);
        GlobalOpenGL().assertNoErrors();

        // Store state values
        current.alphaFunc = _state.alphaFunc;
        current.alphaThreshold = _state.alphaThreshold;
    }

    // Apply polygon offset
    if (_state.polygonOffset != current.polygonOffset)
    {
        current.polygonOffset = _state.polygonOffset;

        if (current.polygonOffset > 0.0f)
        {
            glEnable(GL_POLYGON_OFFSET_FILL);
            glPolygonOffset(-1, -1 *_state.polygonOffset);
        }
        else
        {
            glDisable(GL_POLYGON_OFFSET_FILL);
        }
    }

    // Apply the GL textures
    applyAllTextures(current, requiredState);

    // Set the GL colour if it isn't set already
	if (_state.stage0)
	{
		_state.m_colour = _state.stage0->getColour();
	}

    if (_state.m_colour != current.m_colour)
    {
        glColor4fv(_state.m_colour);
        current.m_colour = _state.m_colour;
        GlobalOpenGL().assertNoErrors();
    }

    // Set up the cubemap and texgen parameters
    setUpCubeMapAndTexGen(current, requiredState, viewer);

  if(requiredState & RENDER_BLEND
    && (_state.m_blend_src != current.m_blend_src || _state.m_blend_dst != current.m_blend_dst))
  {
    glBlendFunc(_state.m_blend_src, _state.m_blend_dst);
    GlobalOpenGL().assertNoErrors();
    current.m_blend_src = _state.m_blend_src;
    current.m_blend_dst = _state.m_blend_dst;
  }

  if(!(requiredState & RENDER_FILL)
    && _state.m_linewidth != current.m_linewidth)
  {
    glLineWidth(_state.m_linewidth);
    GlobalOpenGL().assertNoErrors();
    current.m_linewidth = _state.m_linewidth;
  }

  if(!(requiredState & RENDER_FILL)
    && _state.m_pointsize != current.m_pointsize)
  {
    glPointSize(_state.m_pointsize);
    GlobalOpenGL().assertNoErrors();
    current.m_pointsize = _state.m_pointsize;
  }

  current.renderFlags = requiredState;

  GlobalOpenGL().assertNoErrors();
}

// Add a Renderable to this bucket
void OpenGLShaderPass::addRenderable(const OpenGLRenderable& renderable,
									  const Matrix4& modelview,
									  const RendererLight* light)
{
	_renderablesWithoutEntity.push_back(TransformedRenderable(renderable, modelview, light, NULL));
}

void OpenGLShaderPass::addRenderable(const OpenGLRenderable& renderable,
									  const Matrix4& modelview,
									  const IRenderEntity& entity,
									  const RendererLight* light)
{
	RenderablesByEntity::iterator i = _renderables.find(&entity);

	if (i == _renderables.end())
	{
		i = _renderables.insert(RenderablesByEntity::value_type(&entity, Renderables())).first;
	}

	i->second.push_back(TransformedRenderable(renderable, modelview, light, &entity));
}

// Render the bucket contents
void OpenGLShaderPass::render(OpenGLState& current,
                              unsigned int flagsMask,
                              const Vector3& viewer,
							  std::size_t time)
{
	// Reset the texture matrix
    glMatrixMode(GL_TEXTURE);
	glLoadMatrixf(Matrix4::getIdentity());

    glMatrixMode(GL_MODELVIEW);

	// Apply our state to the current state object
	applyState(current, flagsMask, viewer, time, NULL);

    // If RENDER_SCREEN is set, just render a quad, otherwise render all
    // objects.
    if ((flagsMask & _state.renderFlags & RENDER_SCREEN) != 0)
    {
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadMatrixf(Matrix4::getIdentity());

        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadMatrixf(Matrix4::getIdentity());

        glBegin(GL_QUADS);
        glVertex3f(-1, -1, 0);
        glVertex3f(1, -1, 0);
        glVertex3f(1, 1, 0);
        glVertex3f(-1, 1, 0);
        glEnd();

        glMatrixMode(GL_PROJECTION);
        glPopMatrix();

        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();
    }
	else 
    {
		if (!_renderablesWithoutEntity.empty())
		{
			renderAllContained(_renderablesWithoutEntity, current, viewer, time);
		}

		for (RenderablesByEntity::const_iterator i = _renderables.begin();
             i != _renderables.end();
             ++i)
		{
			// Apply our state to the current state object
			applyState(current, flagsMask, viewer, time, i->first);

			if (!stateIsActive())
			{
				continue;
			}

			renderAllContained(i->second, current, viewer, time);
		}
	}

	_renderablesWithoutEntity.clear();
	_renderables.clear();
}

bool OpenGLShaderPass::stateIsActive()
{
	return ((_state.stage0 == NULL || _state.stage0->isVisible()) && 
			(_state.stage1 == NULL || _state.stage1->isVisible()) && 
			(_state.stage2 == NULL || _state.stage2->isVisible()) && 
			(_state.stage3 == NULL || _state.stage3->isVisible()));
}

// Setup lighting
void OpenGLShaderPass::setUpLightingCalculation(OpenGLState& current,
                                                const RendererLight* light,
                                                const Vector3& viewer,
                                                const Matrix4& objTransform,
												std::size_t time)
{
    assert(light);

    // Get the light shader and examine its first (and only valid) layer
    const MaterialPtr& lightShader = light->getShader()->getMaterial();
	ShaderLayer* layer = lightShader->firstLayer();

    if (layer)
    {
		// Calculate viewer location in object space
        Matrix4 inverseObjTransform = objTransform.getInverse();
        Vector3 osViewer = inverseObjTransform.transformPoint(viewer);

		// Calculate all dynamic values in the layer
		layer->evaluateExpressions(time, *light);

        // Get the XY and Z falloff texture numbers.
        GLuint attenuation_xy = layer->getTexture()->getGLTexNum();
        GLuint attenuation_z = lightShader->lightFalloffImage()->getGLTexNum();

        // Bind the falloff textures
        assert(current.renderFlags & RENDER_TEXTURE_2D);

        setTextureState(
            current.texture3, attenuation_xy, GL_TEXTURE3, GL_TEXTURE_2D
        );
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

        setTextureState(
            current.texture4, attenuation_z, GL_TEXTURE4, GL_TEXTURE_2D
        );
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

		// Get the world-space to light-space transformation matrix
        Matrix4 world2light = light->getLightTextureTransformation();

        // Set the ambient factor - 1.0 for an ambient light, 0.0 for normal light
        float ambient = lightShader->isAmbientLight() ? 1.0f : 0.0f;

        // Bind the GL program parameters
        current.glProgram->applyRenderParams(
            osViewer,
            objTransform,
            light->getLightOrigin(),
            layer->getColour(),
            world2light,
            ambient
        );
    }
}

// Flush renderables
void OpenGLShaderPass::renderAllContained(const Renderables& renderables,
										  OpenGLState& current,
                                          const Vector3& viewer,
										  std::size_t time)
{
	// Keep a pointer to the last transform matrix and render entity used
	const Matrix4* transform = 0;

	glPushMatrix();

	// Iterate over each transformed renderable in the vector
	for(Renderables::const_iterator i = renderables.begin(); i != renderables.end(); ++i)
	{
		// If the current iteration's transform matrix was different from the
		// last, apply it and store for the next iteration
	    if (transform == NULL || 
			(transform != i->transform && !transform->isAffineEqual(*i->transform)))
		{
			transform = i->transform;
      		glPopMatrix();
      		glPushMatrix();
      		glMultMatrixf(*transform);

      		// Determine the face direction
      		if ((current.renderFlags & RENDER_CULLFACE) != 0 && 
				transform->getHandedness() == Matrix4::RIGHTHANDED)
      		{
      			glFrontFace(GL_CW);
      		}
      		else
      		{
      			glFrontFace(GL_CCW);
      		}
    	}

		// If we are using a lighting program and this renderable is lit, set
		// up the lighting calculation
		const RendererLight* light = i->light;
		if (current.glProgram != 0 && light != NULL)
        {
            setUpLightingCalculation(current, light, viewer, *transform, time);
        }

        // Render the renderable
        RenderInfo info(current.renderFlags, viewer, current.cubeMapMode);
        i->renderable->render(info);
    }

    // Cleanup
    glPopMatrix();
}

// Stream insertion operator
std::ostream& operator<<(std::ostream& st, const OpenGLShaderPass& self)
{
// Shortcut to save some typing
#define OUTPUT_RENDERFLAG(x) if (self._state.renderFlags & (x)) { st << "|" << #x; }

	const MaterialPtr& material = self._owner.getMaterial();

	st << (material ? material->getName() : "null material") << " - ";
	
	st << "Renderflags: ";

	OUTPUT_RENDERFLAG(RENDER_LINESTIPPLE);
	OUTPUT_RENDERFLAG(RENDER_LINESMOOTH);
	OUTPUT_RENDERFLAG(RENDER_POLYGONSTIPPLE);
	OUTPUT_RENDERFLAG(RENDER_POLYGONSMOOTH);
	OUTPUT_RENDERFLAG(RENDER_ALPHATEST);
	OUTPUT_RENDERFLAG(RENDER_DEPTHTEST);
	OUTPUT_RENDERFLAG(RENDER_DEPTHWRITE);
	OUTPUT_RENDERFLAG(RENDER_COLOURWRITE);
	OUTPUT_RENDERFLAG(RENDER_CULLFACE);
	OUTPUT_RENDERFLAG(RENDER_SCALED);
	OUTPUT_RENDERFLAG(RENDER_SMOOTH);
	OUTPUT_RENDERFLAG(RENDER_LIGHTING);
	OUTPUT_RENDERFLAG(RENDER_BLEND);
	OUTPUT_RENDERFLAG(RENDER_OFFSETLINE);
	OUTPUT_RENDERFLAG(RENDER_FILL);
	OUTPUT_RENDERFLAG(RENDER_COLOURARRAY);
	OUTPUT_RENDERFLAG(RENDER_COLOURCHANGE);
	OUTPUT_RENDERFLAG(RENDER_MATERIAL_VCOL);
	OUTPUT_RENDERFLAG(RENDER_VCOL_INVERT);
	OUTPUT_RENDERFLAG(RENDER_TEXTURE_2D);
	OUTPUT_RENDERFLAG(RENDER_TEXTURE_CUBEMAP);
	OUTPUT_RENDERFLAG(RENDER_BUMP);
	OUTPUT_RENDERFLAG(RENDER_PROGRAM);
	OUTPUT_RENDERFLAG(RENDER_SCREEN);
	OUTPUT_RENDERFLAG(RENDER_OVERRIDE);

	st << " - ";

	st << "Sort: " << self._state.m_sort << " - ";
	st << "PolygonOffset: " << self._state.polygonOffset << " - ";

	if (self._state.texture0 > 0) st << "Texture0: " << self._state.texture0 << " - ";
	if (self._state.texture1 > 0) st << "Texture1: " << self._state.texture1 << " - ";
	if (self._state.texture2 > 0) st << "Texture2: " << self._state.texture2 << " - ";
	if (self._state.texture3 > 0) st << "Texture3: " << self._state.texture3 << " - ";
	if (self._state.texture4 > 0) st << "Texture4: " << self._state.texture4 << " - ";

	st << "Colour: " << self._state.m_colour << " - ";

	st << "CubeMapMode: " << self._state.cubeMapMode;

	st << std::endl;

	return st;
}
