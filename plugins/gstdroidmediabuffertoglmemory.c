/* GStreamer
 * Copyright (C) 2019 FIXME <fixme@example.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Suite 500,
 * Boston, MA 02110-1335, USA.
 */
/**
 * SECTION:element-gstdroidmediabuffertoglmemory
 *
 * The droidmediabuffertoglmemory element does FIXME stuff.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch-1.0 -v fakesrc ! droidmediabuffertoglmemory ! FIXME ! fakesink
 * ]|
 * FIXME Describe what the pipeline does.
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include <gst/base/gstbasetransform.h>
#include <gst/gl/gstglutils.h>
#include <gst/gl/gstglmemory.h>
#include <gst/allocators/gstdroidmediabuffer.h>

#include "gstdroidmediabuffertoglmemory.h"

GST_DEBUG_CATEGORY_STATIC (gst_droidmediabuffertoglmemory_debug_category);
#define GST_CAT_DEFAULT gst_droidmediabuffertoglmemory_debug_category

/* prototypes */


static void gst_droidmediabuffertoglmemory_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec);
static void gst_droidmediabuffertoglmemory_get_property (GObject * object,
    guint property_id, GValue * value, GParamSpec * pspec);
static void gst_droidmediabuffertoglmemory_dispose (GObject * object);
static void gst_droidmediabuffertoglmemory_finalize (GObject * object);

static void gst_droidmediabuffertoglmemory_set_context (GstElement * element,
    GstContext * context);

static GstCaps *gst_droidmediabuffertoglmemory_transform_caps (GstBaseTransform
    * trans, GstPadDirection direction, GstCaps * caps, GstCaps * filter);
static gboolean
gst_droidmediabuffertoglmemory_decide_allocation (GstBaseTransform * trans,
    GstQuery * query);
static gboolean gst_droidmediabuffertoglmemory_filter_meta (GstBaseTransform *
    trans, GstQuery * query, GType api, const GstStructure * params);
static gboolean gst_droidmediabuffertoglmemory_transform_size (GstBaseTransform
    * trans, GstPadDirection direction, GstCaps * caps, gsize size,
    GstCaps * othercaps, gsize * othersize);
static gboolean gst_droidmediabuffertoglmemory_get_unit_size (GstBaseTransform *
    trans, GstCaps * caps, gsize * size);
static gboolean gst_droidmediabuffertoglmemory_start (GstBaseTransform * trans);
static gboolean gst_droidmediabuffertoglmemory_stop (GstBaseTransform * trans);
static gboolean gst_droidmediabuffertoglmemory_sink_event (GstBaseTransform *
    trans, GstEvent * event);
static gboolean gst_droidmediabuffertoglmemory_src_event (GstBaseTransform *
    trans, GstEvent * event);
static GstFlowReturn
gst_droidmediabuffertoglmemory_prepare_output_buffer (GstBaseTransform * trans,
    GstBuffer * input, GstBuffer ** outbuf);
static gboolean gst_droidmediabuffertoglmemory_copy_metadata (GstBaseTransform *
    trans, GstBuffer * input, GstBuffer * outbuf);
static gboolean gst_droidmediabuffertoglmemory_transform_meta (GstBaseTransform
    * trans, GstBuffer * outbuf, GstMeta * meta, GstBuffer * inbuf);
static void gst_droidmediabuffertoglmemory_before_transform (GstBaseTransform *
    trans, GstBuffer * buffer);
static GstFlowReturn gst_droidmediabuffertoglmemory_transform (GstBaseTransform
    * trans, GstBuffer * inbuf, GstBuffer * outbuf);

enum
{
  PROP_0
};

/* pad templates */

static GstStaticPadTemplate gst_droidmediabuffertoglmemory_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE_WITH_FEATURES
        (GST_CAPS_FEATURE_MEMORY_GL_MEMORY, "{ RGBA }") // XXX: is "RGBA" correct?
        ", texture-target = (string) { 2D }"
        )
    );

static GstStaticPadTemplate gst_droidmediabuffertoglmemory_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE_WITH_FEATURES
        (GST_CAPS_FEATURE_MEMORY_DROID_MEDIA_BUFFER,
            GST_DROID_MEDIA_BUFFER_MEMORY_VIDEO_FORMATS)));


/* class initialization */

G_DEFINE_TYPE_WITH_CODE (GstDroidmediabuffertoglmemory,
    gst_droidmediabuffertoglmemory, GST_TYPE_BASE_TRANSFORM,
    GST_DEBUG_CATEGORY_INIT (gst_droidmediabuffertoglmemory_debug_category,
        "droidmediabuffertoglmemory", 0,
        "debug category for droidmediabuffertoglmemory element"));

static void
gst_droidmediabuffertoglmemory_class_init (GstDroidmediabuffertoglmemoryClass *
    klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstElementClass *gstelement_class = (GstElementClass *) klass;
  GstBaseTransformClass *base_transform_class =
      GST_BASE_TRANSFORM_CLASS (klass);

  /* Setting up pads and setting metadata should be moved to
     base_class_init if you intend to subclass this class. */
  gst_element_class_add_static_pad_template (GST_ELEMENT_CLASS (klass),
      &gst_droidmediabuffertoglmemory_src_template);
  gst_element_class_add_static_pad_template (GST_ELEMENT_CLASS (klass),
      &gst_droidmediabuffertoglmemory_sink_template);

  gst_element_class_set_static_metadata (GST_ELEMENT_CLASS (klass),
      "FIXME Long name", "Generic", "FIXME Description",
      "FIXME <fixme@example.com>");

  gobject_class->set_property = gst_droidmediabuffertoglmemory_set_property;
  gobject_class->get_property = gst_droidmediabuffertoglmemory_get_property;
  gobject_class->dispose = gst_droidmediabuffertoglmemory_dispose;
  gobject_class->finalize = gst_droidmediabuffertoglmemory_finalize;
  gstelement_class->set_context = 
      GST_DEBUG_FUNCPTR(gst_droidmediabuffertoglmemory_set_context);
  base_transform_class->transform_caps =
      GST_DEBUG_FUNCPTR (gst_droidmediabuffertoglmemory_transform_caps);
  base_transform_class->decide_allocation =
      GST_DEBUG_FUNCPTR (gst_droidmediabuffertoglmemory_decide_allocation);
  base_transform_class->filter_meta =
      GST_DEBUG_FUNCPTR (gst_droidmediabuffertoglmemory_filter_meta);
  base_transform_class->transform_size =
      GST_DEBUG_FUNCPTR (gst_droidmediabuffertoglmemory_transform_size);
  base_transform_class->get_unit_size =
      GST_DEBUG_FUNCPTR (gst_droidmediabuffertoglmemory_get_unit_size);
  base_transform_class->start =
      GST_DEBUG_FUNCPTR (gst_droidmediabuffertoglmemory_start);
  base_transform_class->stop =
      GST_DEBUG_FUNCPTR (gst_droidmediabuffertoglmemory_stop);
  base_transform_class->sink_event =
      GST_DEBUG_FUNCPTR (gst_droidmediabuffertoglmemory_sink_event);
  base_transform_class->src_event =
      GST_DEBUG_FUNCPTR (gst_droidmediabuffertoglmemory_src_event);
  base_transform_class->prepare_output_buffer =
      GST_DEBUG_FUNCPTR (gst_droidmediabuffertoglmemory_prepare_output_buffer);
  base_transform_class->copy_metadata =
      GST_DEBUG_FUNCPTR (gst_droidmediabuffertoglmemory_copy_metadata);
  base_transform_class->transform_meta =
      GST_DEBUG_FUNCPTR (gst_droidmediabuffertoglmemory_transform_meta);
  base_transform_class->before_transform =
      GST_DEBUG_FUNCPTR (gst_droidmediabuffertoglmemory_before_transform);
  base_transform_class->transform =
      GST_DEBUG_FUNCPTR (gst_droidmediabuffertoglmemory_transform);

}

static void
gst_droidmediabuffertoglmemory_init (GstDroidmediabuffertoglmemory *
    droidmediabuffertoglmemory)
{
  droidmediabuffertoglmemory->display = NULL;
  droidmediabuffertoglmemory->context = NULL;
  droidmediabuffertoglmemory->other_context = NULL;
}

void
gst_droidmediabuffertoglmemory_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec)
{
  GstDroidmediabuffertoglmemory *droidmediabuffertoglmemory =
      GST_DROIDMEDIABUFFERTOGLMEMORY (object);

  GST_DEBUG_OBJECT (droidmediabuffertoglmemory, "set_property");

  switch (property_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void
gst_droidmediabuffertoglmemory_get_property (GObject * object,
    guint property_id, GValue * value, GParamSpec * pspec)
{
  GstDroidmediabuffertoglmemory *droidmediabuffertoglmemory =
      GST_DROIDMEDIABUFFERTOGLMEMORY (object);

  GST_DEBUG_OBJECT (droidmediabuffertoglmemory, "get_property");

  switch (property_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void
gst_droidmediabuffertoglmemory_dispose (GObject * object)
{
  GstDroidmediabuffertoglmemory *droidmediabuffertoglmemory =
      GST_DROIDMEDIABUFFERTOGLMEMORY (object);

  GST_DEBUG_OBJECT (droidmediabuffertoglmemory, "dispose");

  /* clean up as possible.  may be called multiple times */

  G_OBJECT_CLASS (gst_droidmediabuffertoglmemory_parent_class)->
      dispose (object);
}

void
gst_droidmediabuffertoglmemory_finalize (GObject * object)
{
  GstDroidmediabuffertoglmemory *droidmediabuffertoglmemory =
      GST_DROIDMEDIABUFFERTOGLMEMORY (object);

  GST_DEBUG_OBJECT (droidmediabuffertoglmemory, "finalize");

  /* clean up object here */

  G_OBJECT_CLASS (gst_droidmediabuffertoglmemory_parent_class)->
      finalize (object);
}

static void
gst_droidmediabuffertoglmemory_set_context (GstElement * element, GstContext * context)
{
  // Pretty much copied from gstglimagesink.c

  GstDroidmediabuffertoglmemory *droidmediabuffertoglmemory =
      GST_DROIDMEDIABUFFERTOGLMEMORY (element);

  gst_gl_handle_set_context (element, context,
          &droidmediabuffertoglmemory->display,
          &droidmediabuffertoglmemory->other_context
  );

  // TODO: Is this needed?
  #if 0
  if (droidmediabuffertoglmemory->display)
    gst_gl_display_filter_gl_api (droidmediabuffertoglmemory->display, SUPPORTED_GL_APIS);
  #endif

  GST_ELEMENT_CLASS (gst_droidmediabuffertoglmemory_parent_class)->set_context (element, context);
}

static GstCaps *
gst_droidmediabuffertoglmemory_transform_caps (GstBaseTransform * trans,
    GstPadDirection direction, GstCaps * caps, GstCaps * filter)
{
  GstDroidmediabuffertoglmemory *droidmediabuffertoglmemory =
      GST_DROIDMEDIABUFFERTOGLMEMORY (trans);
  GstCaps *othercaps;

  GST_DEBUG_OBJECT (droidmediabuffertoglmemory, "transform_caps");

  if (direction == GST_PAD_SRC) {
    /* transform caps going upstream */

    // I don't want to deal with it. Pad template should be enough.
    othercaps = gst_caps_new_any();
  } else {
    /* transform caps going downstream */

    othercaps = gst_caps_copy (caps);

    // Replace the feature with GLMemory & set color-format to RGBA
    // Also set texture-target
    GstCapsFeatures * glmemoryfeat = gst_caps_features_new(
        GST_CAPS_FEATURE_MEMORY_GL_MEMORY,
        NULL
    );
    gst_caps_set_features(othercaps, /* index (XXX: other index?) */ 0, glmemoryfeat);
    
    gst_caps_set_simple(othercaps,
                            "format", G_TYPE_STRING, "RGBA",
                            "texture-target", G_TYPE_STRING, "2D",
                            NULL);
  }

  if (filter) {
    GstCaps *intersect;

    intersect = gst_caps_intersect (othercaps, filter);
    gst_caps_unref (othercaps);

    return intersect;
  } else {
    return othercaps;
  }
}

/* decide allocation query for output buffers */
static gboolean
gst_droidmediabuffertoglmemory_decide_allocation (GstBaseTransform * trans,
    GstQuery * query)
{
  GstDroidmediabuffertoglmemory *droidmediabuffertoglmemory =
      GST_DROIDMEDIABUFFERTOGLMEMORY (trans);

  GST_DEBUG_OBJECT (droidmediabuffertoglmemory, "decide_allocation");

  return TRUE;
}

static gboolean
gst_droidmediabuffertoglmemory_filter_meta (GstBaseTransform * trans,
    GstQuery * query, GType api, const GstStructure * params)
{
  GstDroidmediabuffertoglmemory *droidmediabuffertoglmemory =
      GST_DROIDMEDIABUFFERTOGLMEMORY (trans);

  GST_DEBUG_OBJECT (droidmediabuffertoglmemory, "filter_meta");

  return TRUE;
}

/* transform size */
static gboolean
gst_droidmediabuffertoglmemory_transform_size (GstBaseTransform * trans,
    GstPadDirection direction, GstCaps * caps, gsize size, GstCaps * othercaps,
    gsize * othersize)
{
  GstDroidmediabuffertoglmemory *droidmediabuffertoglmemory =
      GST_DROIDMEDIABUFFERTOGLMEMORY (trans);

  GST_DEBUG_OBJECT (droidmediabuffertoglmemory, "transform_size");

  return TRUE;
}

static gboolean
gst_droidmediabuffertoglmemory_get_unit_size (GstBaseTransform * trans,
    GstCaps * caps, gsize * size)
{
  GstDroidmediabuffertoglmemory *droidmediabuffertoglmemory =
      GST_DROIDMEDIABUFFERTOGLMEMORY (trans);

  GST_DEBUG_OBJECT (droidmediabuffertoglmemory, "get_unit_size");

  return TRUE;
}

static gboolean
_ensure_gl_context (GstDroidmediabuffertoglmemory * droidmediabuffertoglmemory)
{
  // Pretty much copied from gstglimagesink.c, but only for context part.
  GError *error = NULL;

  GST_TRACE_OBJECT (droidmediabuffertoglmemory, "Ensuring GL context");

  if (!droidmediabuffertoglmemory->context) {
    GST_OBJECT_LOCK (droidmediabuffertoglmemory->display);
    do {
      GstGLContext *other_context = NULL;

      if (droidmediabuffertoglmemory->context) {
        gst_object_unref (droidmediabuffertoglmemory->context);
        droidmediabuffertoglmemory->context = NULL;
      }

      GST_DEBUG_OBJECT (droidmediabuffertoglmemory,
          "No current context, creating one for %" GST_PTR_FORMAT,
          droidmediabuffertoglmemory->display);

      if (droidmediabuffertoglmemory->other_context) {
        other_context =
            gst_object_ref (droidmediabuffertoglmemory->other_context);
      } else {
        other_context =
            gst_gl_display_get_gl_context_for_thread
            (droidmediabuffertoglmemory->display, NULL);
      }

      if (!gst_gl_display_create_context (droidmediabuffertoglmemory->display,
              other_context, &droidmediabuffertoglmemory->context, &error)) {
        if (other_context)
          gst_object_unref (other_context);
        GST_OBJECT_UNLOCK (droidmediabuffertoglmemory->display);
        goto context_error;
      }

      GST_DEBUG_OBJECT (droidmediabuffertoglmemory,
          "created context %" GST_PTR_FORMAT " from other context %"
          GST_PTR_FORMAT, droidmediabuffertoglmemory->context,
          droidmediabuffertoglmemory->other_context);
    } while (!gst_gl_display_add_context (droidmediabuffertoglmemory->display,
            droidmediabuffertoglmemory->context));
    GST_OBJECT_UNLOCK (droidmediabuffertoglmemory->display);
  }
  return TRUE;

context_error:
  {
    GST_ELEMENT_ERROR (droidmediabuffertoglmemory, RESOURCE, NOT_FOUND, ("%s",
            error->message), (NULL));

    if (droidmediabuffertoglmemory->context) {
      gst_object_unref (droidmediabuffertoglmemory->context);
      droidmediabuffertoglmemory->context = NULL;
    }

    g_clear_error (&error);

    return FALSE;
  }
}

/* states */
static gboolean
gst_droidmediabuffertoglmemory_start (GstBaseTransform * trans)
{
  GstDroidmediabuffertoglmemory *droidmediabuffertoglmemory =
      GST_DROIDMEDIABUFFERTOGLMEMORY (trans);

  GST_DEBUG_OBJECT (droidmediabuffertoglmemory, "start");

  return TRUE;
}

static gboolean
gst_droidmediabuffertoglmemory_stop (GstBaseTransform * trans)
{
  GstDroidmediabuffertoglmemory *droidmediabuffertoglmemory =
      GST_DROIDMEDIABUFFERTOGLMEMORY (trans);

  GST_DEBUG_OBJECT (droidmediabuffertoglmemory, "stop");

  return TRUE;
}

/* sink and src pad event handlers */
static gboolean
gst_droidmediabuffertoglmemory_sink_event (GstBaseTransform * trans,
    GstEvent * event)
{
  GstDroidmediabuffertoglmemory *droidmediabuffertoglmemory =
      GST_DROIDMEDIABUFFERTOGLMEMORY (trans);

  GST_DEBUG_OBJECT (droidmediabuffertoglmemory, "sink_event");

  return
      GST_BASE_TRANSFORM_CLASS (gst_droidmediabuffertoglmemory_parent_class)->
      sink_event (trans, event);
}

static gboolean
gst_droidmediabuffertoglmemory_src_event (GstBaseTransform * trans,
    GstEvent * event)
{
  GstDroidmediabuffertoglmemory *droidmediabuffertoglmemory =
      GST_DROIDMEDIABUFFERTOGLMEMORY (trans);

  GST_DEBUG_OBJECT (droidmediabuffertoglmemory, "src_event");

  return
      GST_BASE_TRANSFORM_CLASS (gst_droidmediabuffertoglmemory_parent_class)->
      src_event (trans, event);
}

static GstFlowReturn
gst_droidmediabuffertoglmemory_prepare_output_buffer (GstBaseTransform * trans,
    GstBuffer * input, GstBuffer ** outbuf)
{
  GstDroidmediabuffertoglmemory *droidmediabuffertoglmemory =
      GST_DROIDMEDIABUFFERTOGLMEMORY (trans);

  GST_DEBUG_OBJECT (droidmediabuffertoglmemory, "prepare_output_buffer");

  return GST_FLOW_OK;
}

/* metadata */
static gboolean
gst_droidmediabuffertoglmemory_copy_metadata (GstBaseTransform * trans,
    GstBuffer * input, GstBuffer * outbuf)
{
  GstDroidmediabuffertoglmemory *droidmediabuffertoglmemory =
      GST_DROIDMEDIABUFFERTOGLMEMORY (trans);

  GST_DEBUG_OBJECT (droidmediabuffertoglmemory, "copy_metadata");

  return TRUE;
}

static gboolean
gst_droidmediabuffertoglmemory_transform_meta (GstBaseTransform * trans,
    GstBuffer * outbuf, GstMeta * meta, GstBuffer * inbuf)
{
  GstDroidmediabuffertoglmemory *droidmediabuffertoglmemory =
      GST_DROIDMEDIABUFFERTOGLMEMORY (trans);

  GST_DEBUG_OBJECT (droidmediabuffertoglmemory, "transform_meta");

  return TRUE;
}

static void
gst_droidmediabuffertoglmemory_before_transform (GstBaseTransform * trans,
    GstBuffer * buffer)
{
  GstDroidmediabuffertoglmemory *droidmediabuffertoglmemory =
      GST_DROIDMEDIABUFFERTOGLMEMORY (trans);

  GST_DEBUG_OBJECT (droidmediabuffertoglmemory, "before_transform");

}

/* transform */
static GstFlowReturn
gst_droidmediabuffertoglmemory_transform (GstBaseTransform * trans,
    GstBuffer * inbuf, GstBuffer * outbuf)
{
  GstDroidmediabuffertoglmemory *droidmediabuffertoglmemory =
      GST_DROIDMEDIABUFFERTOGLMEMORY (trans);

  GST_DEBUG_OBJECT (droidmediabuffertoglmemory, "transform");

  return GST_FLOW_OK;
}
