/*******************************************************************************
**3456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 
**      10        20        30        40        50        60        70        80
**
** program:
**    cairo-clock
**
** author:
**    Mirco "MacSlow" Müller <macslow@bangang.de>, <macslow@gmail.com>
**
** created: .
**    10.1.2006 (or so)
**
** last change:
**    17.8.2007
**
** notes:
**    In my ongoing efforts to do something useful while learning the cairo-API
**    I produced this nifty program. Surprisingly it displays the current system
**    time in the old-fashioned way of an analog clock. I place this program
**    under the "GNU General Public License". If you don't know what that means
**    take a look a here...
**
**        http://www.gnu.org/licenses/licenses.html#GPL
**
** todo:
**    clean up code and make it sane to read/understand
**
** supplied patches:
**    26.3.2006 - received a patch to add a 24h-mode from Darryll "Moppsy"
**    Truchan <moppsy@comcast.net>
**
*******************************************************************************/

#include "config.h"

#include <time.h>
#include <math.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <ctype.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <glade/glade.h>
#include <librsvg/rsvg.h>
#include <librsvg/rsvg-cairo.h>
#include <glib/gi18n.h>

#if !GTK_CHECK_VERSION(2,9,0)
#include <X11/Xlib.h>
#include <X11/extensions/shape.h>
#include <gdk/gdkx.h>
#endif

#define SECOND_INTERVAL	 1000
#define MINUTE_INTERVAL	60000
#define MIN_WIDTH	   32
#define MIN_HEIGHT	   32
#define MAX_WIDTH	 1023
#define MAX_HEIGHT	 1023
#define MIN_REFRESH_RATE    1
#define MAX_REFRESH_RATE   60

typedef enum _LayerElement
{
	CLOCK_DROP_SHADOW = 0,
	CLOCK_FACE,
	CLOCK_MARKS,
	CLOCK_HOUR_HAND_SHADOW,
	CLOCK_MINUTE_HAND_SHADOW,
	CLOCK_SECOND_HAND_SHADOW,
	CLOCK_HOUR_HAND,
	CLOCK_MINUTE_HAND,
	CLOCK_SECOND_HAND,
	CLOCK_FACE_SHADOW,
	CLOCK_GLASS,
	CLOCK_FRAME,
	CLOCK_ELEMENTS
} LayerElement;

typedef enum _SurfaceKind
{
	KIND_BACKGROUND = 0,
	KIND_FOREGROUND
} SurfaceKind;

typedef enum _StartupSizeKind
{
	SIZE_SMALL = 0,
	SIZE_MEDIUM,
	SIZE_LARGE,
	SIZE_CUSTOM
} StartupSizeKind;

typedef struct _ThemeEntry
{
	GString* pName;
	GString* pPath;
} ThemeEntry;

/* yeah I know... global variables are the devil */
cairo_t*		g_pMainContext;
RsvgHandle*		g_pSvgHandles[CLOCK_ELEMENTS];
char			g_cFileNames[CLOCK_ELEMENTS][30] =
{
	"clock-drop-shadow.svg",
	"clock-face.svg",
	"clock-marks.svg",
	"clock-hour-hand-shadow.svg",
	"clock-minute-hand-shadow.svg",
	"clock-second-hand-shadow.svg",
	"clock-hour-hand.svg",
	"clock-minute-hand.svg",
	"clock-second-hand.svg",
	"clock-face-shadow.svg",
	"clock-glass.svg",
	"clock-frame.svg"
};

RsvgDimensionData g_DimensionData;
gint		  g_iSeconds;
gint		  g_iMinutes;
gint		  g_iHours;
gint		  g_iDay;
gint		  g_iMonth;
gchar		  g_acDate[6];
static time_t	  g_timeOfDay;
struct tm*	  g_pTime;
gint		  g_i12			 = 0;	/* 12h hour-hand toggle       */
gint		  g_i24			 = 0;	/* 24h hour-hand toggle       */
gint		  g_iShowDate		 = 0;	/* date-display toggle        */
gint		  g_iShowSeconds	 = 0;	/* seconds-hand toggle        */
gint		  g_iDefaultX		 = -1;	/* x-pos of top-left corner   */
gint		  g_iDefaultY		 = -1;	/* y-pos, < 0 means undefined */
gint		  g_iDefaultWidth	 = 127;	/* clock-window used width ...*/
gint		  g_iDefaultHeight	 = 127;	/* ... and height             */
static gchar*	  g_pcTheme;
gchar		  g_acTheme[80]		 = "default\0";
gint		  g_iKeepOnTop		 = 0;
gint		  g_iAppearInPager	 = 0;
gint		  g_iAppearInTaskbar	 = 0;
gint		  g_iSticky		 = 0;
GtkWidget*	  g_pMainWindow		 = NULL;
GtkWidget*	  g_pPopUpMenu		 = NULL;
GtkWidget*	  g_pSettingsDialog	 = NULL;
GtkWidget*	  g_pInfoDialog		 = NULL;
GtkWidget*	  g_pErrorDialog	 = NULL;
GtkWidget*	  g_pTableStartupSize	 = NULL;
GtkWidget*	  g_pComboBoxStartupSize = NULL;
GtkWidget*	  g_pSpinButtonWidth	 = NULL;
GtkWidget*	  g_pSpinButtonHeight	 = NULL;
GtkWidget*	  g_pHScaleSmoothness	 = NULL;
guint		  g_iuIntervalHandlerId	 = 0;
gint		  g_iThemeCounter	 = 0;
GList*		  g_pThemeList		 = NULL;
gchar		  g_acAppName[]		 = "MacSlow's Cairo-Clock";
gchar		  g_acAppVersion[]	 = "0.3.4";
gboolean	  g_bNeedsUpdate	 = TRUE;
cairo_surface_t*  g_pBackgroundSurface	 = NULL;
cairo_surface_t*  g_pForegroundSurface	 = NULL;
gint		  g_iRefreshRate	 = 30;
GTimer*		  g_pTimer		 = NULL;
gboolean	  bPrintThemeList	 = FALSE;
gboolean	  bPrintVersion		 = FALSE;

static void
render (gint iWidth,
	gint iHeight);

static void
update_input_shape (GtkWidget* pWindow,
		    gint       iWidth,
		    gint       iHeight);

static gboolean
is_power_of_two (gint iValue)
{
	gint	 iExponent = 0;
	gboolean bResult   = FALSE;

	for (iExponent = 1; iExponent <= 32; iExponent++)
		if ((unsigned long) iValue - (1L << iExponent) == 0)
		{
			bResult = TRUE;
			break;
		}

	return bResult;
}

static void
on_info_close (GtkDialog* pDialog,
	       gpointer   data)
{
	gtk_widget_hide (GTK_WIDGET (pDialog));
}

static void
draw_background (cairo_t* pDrawingContext,
		 gint	  iWidth,
		 gint	  iHeight)
{
	/* clear context */
	cairo_scale (pDrawingContext,
		     (double) iWidth / (double) g_DimensionData.width,
		     (double) iHeight / (double) g_DimensionData.height);
	cairo_set_source_rgba (pDrawingContext, 1.0f, 1.0f, 1.0f, 0.0f);
	cairo_set_operator (pDrawingContext, CAIRO_OPERATOR_OVER);
	cairo_paint (pDrawingContext);

	/* draw stuff */
	rsvg_handle_render_cairo (g_pSvgHandles[CLOCK_DROP_SHADOW],
				  pDrawingContext);
	rsvg_handle_render_cairo (g_pSvgHandles[CLOCK_FACE],
				  pDrawingContext);
	rsvg_handle_render_cairo (g_pSvgHandles[CLOCK_MARKS],
				  pDrawingContext);
}

static void
draw_foreground (cairo_t* pDrawingContext,
		 gint	  iWidth,
		 gint	  iHeight)
{
	/* clear context */
	cairo_scale (pDrawingContext,
		     (double) iWidth / (double) g_DimensionData.width,
		     (double) iHeight / (double) g_DimensionData.height);
	cairo_set_source_rgba (pDrawingContext, 1.0f, 1.0f, 1.0f, 0.0f);
	cairo_set_operator (pDrawingContext, CAIRO_OPERATOR_OVER);
	cairo_paint (pDrawingContext);

	/* draw stuff */
	rsvg_handle_render_cairo (g_pSvgHandles[CLOCK_FACE_SHADOW],
				  pDrawingContext);
	rsvg_handle_render_cairo (g_pSvgHandles[CLOCK_GLASS],
				  pDrawingContext);
	rsvg_handle_render_cairo (g_pSvgHandles[CLOCK_FRAME],
				  pDrawingContext);
}

static cairo_surface_t*
update_surface (cairo_surface_t* pOldSurface,
		cairo_t*	 pSourceContext,
		gint		 iWidth,
		gint		 iHeight,
		SurfaceKind	 kind)
{
	cairo_surface_t* pNewSurface = NULL;
	cairo_t* pDrawingContext = NULL;

	cairo_surface_destroy (pOldSurface);
	pNewSurface = cairo_surface_create_similar (cairo_get_target (pSourceContext),
						    CAIRO_CONTENT_COLOR_ALPHA,
						    iWidth,
						    iHeight);
	if (cairo_surface_status (pNewSurface) != CAIRO_STATUS_SUCCESS)
		return NULL;

	pDrawingContext = cairo_create (pNewSurface);
	if (cairo_status (pDrawingContext) != CAIRO_STATUS_SUCCESS)
		return NULL;

	switch (kind)
	{
		case KIND_BACKGROUND :
			draw_background (pDrawingContext, iWidth, iHeight);
		break;

		case KIND_FOREGROUND :
			draw_foreground (pDrawingContext, iWidth, iHeight);
		break;
	}

	cairo_destroy (pDrawingContext);

	return pNewSurface;
}

static gboolean
time_handler (GtkWidget* pWidget)
{
	gtk_widget_queue_draw (pWidget);
	return TRUE;
}

static gboolean
on_expose (GtkWidget*	   pWidget,
	   GdkEventExpose* pExpose)
{
	static gint iWidth;
	static gint iHeight;

	g_pMainContext = gdk_cairo_create (pWidget->window);
	cairo_set_operator (g_pMainContext, CAIRO_OPERATOR_SOURCE);
	gtk_window_get_size (GTK_WINDOW (pWidget), &iWidth, &iHeight);
	if (g_bNeedsUpdate == TRUE)
	{
		g_pBackgroundSurface = update_surface (g_pBackgroundSurface,
						       g_pMainContext,
						       iWidth,
						       iHeight,
						       KIND_BACKGROUND);
		g_pForegroundSurface = update_surface (g_pForegroundSurface,
						       g_pMainContext,
						       iWidth,
						       iHeight,
						       KIND_FOREGROUND);
		g_bNeedsUpdate = FALSE;
	}
	render (iWidth, iHeight);
	cairo_destroy (g_pMainContext);

	return FALSE;
}

static void
on_alpha_screen_changed (GtkWidget* pWidget,
			 GdkScreen* pOldScreen,
			 GtkWidget* pLabel)
{                       
	GdkScreen*   pScreen   = gtk_widget_get_screen (pWidget);
	GdkColormap* pColormap = gdk_screen_get_rgba_colormap (pScreen);
      
	if (!pColormap)
		pColormap = gdk_screen_get_rgb_colormap (pScreen);

	gtk_widget_set_colormap (pWidget, pColormap);
}

static gboolean
on_key_press (GtkWidget*   pWidget,
	      GdkEventKey* pKey,
	      gpointer	   userData)
{
	if (pKey->type == GDK_KEY_PRESS)
	{
		switch (pKey->keyval)
		{
			case GDK_Escape :
				gtk_main_quit ();
			break;
		}
	}

	return FALSE;
}

static gboolean
on_button_press (GtkWidget*	 pWidget,
		 GdkEventButton* pButton,
		 GdkWindowEdge	 edge)
{
	if (pButton->type == GDK_BUTTON_PRESS)
	{
		if (pButton->button == 1)
			gtk_window_begin_move_drag (GTK_WINDOW (gtk_widget_get_toplevel (pWidget)),
						    pButton->button,
						    pButton->x_root,
						    pButton->y_root,
						    pButton->time);

		if (pButton->button == 2)
			gtk_window_begin_resize_drag (GTK_WINDOW (gtk_widget_get_toplevel (pWidget)),
						      edge,
						      pButton->button,
						      pButton->x_root,
						      pButton->y_root,
						      pButton->time);

		if (pButton->button == 3)
			gtk_menu_popup (GTK_MENU (g_pPopUpMenu),
					NULL,
					NULL,
					NULL,
					NULL,
					pButton->button,
					pButton->time);
	}

	return TRUE;
}

static void
on_settings_activate (GtkMenuItem* pMenuItem,
		      gpointer	   data)
{
	gtk_widget_show (g_pSettingsDialog);
}

static void
on_info_activate (GtkMenuItem* pMenuItem,
		  gpointer     data)
{
	gtk_widget_show (g_pInfoDialog);
}

static void
on_quit_activate (GtkMenuItem* pMenuItem,
		  gpointer     data)
{
	gtk_main_quit ();
}

static void
render (gint iWidth,
	gint iHeight)
{
	static double		    fHalfX	      = 0.0f;
	static double		    fHalfY	      = 0.0f;
	static double		    fShadowOffsetX    = -0.75f;
	static double		    fShadowOffsetY    = 0.75f;
	static cairo_text_extents_t textExtents;
	static double		    fFactor	      = 0.0f;
	static gint		    iFrames	      = 0;
	static gulong		    ulMilliSeconds    = 0;
	static double		    fFullSecond	      = 0.0f;
	static double		    fLastFullSecond   = 0.0f;
	static double		    fCurrentTimeStamp = 0.0f;
	static double		    fLastTimeStamp    = 0.0f;
	static double		    fAngleSecond      = 0.0f;
	static double		    fAngleMinute      = 0.0f;
	static double		    fAngleHour        = 0.0f;
	static gboolean		    bAnimateMinute    = FALSE;

	fFactor = powf ((float) iFrames /
			(float) g_iRefreshRate *
			0.875f *
			3.4f,
			3.0f) *
		  sinf ((float) iFrames /
			(float) g_iRefreshRate *
			0.875f *
			G_PI) *
			0.1f;

	fHalfX = g_DimensionData.width / 2.0f;
	fHalfY = g_DimensionData.height / 2.0f;

	time (&g_timeOfDay);
	g_pTime = localtime (&g_timeOfDay);
	g_iSeconds = g_pTime->tm_sec;
	g_iMinutes = g_pTime->tm_min;
	g_iHours = g_pTime->tm_hour;

	if (!bAnimateMinute)
		fAngleMinute = (double) g_iMinutes * 6.0f;

	if (!g_i24)
	{
		g_iHours = g_iHours >= 12 ? g_iHours - 12 : g_iHours;
		fAngleHour = (g_iSeconds + 60 * g_iMinutes + 3600 * (g_iHours % 12)) *
			      360.0 /
			      43200.0f;

	}
	else
		fAngleHour = (g_iSeconds + 60 * g_iMinutes + 3600 * g_iHours) *
			      360.0 /
			      86400.0f;

	g_iDay = g_pTime->tm_mday;
	g_iMonth = g_pTime->tm_mon + 1;
	sprintf (g_acDate, "%02d/%02d", g_iDay, g_iMonth);

	cairo_set_operator (g_pMainContext, CAIRO_OPERATOR_SOURCE);

	cairo_set_source_surface (g_pMainContext,
				  g_pBackgroundSurface,
				  0.0f,
				  0.0f);
	cairo_paint (g_pMainContext);

	cairo_set_operator (g_pMainContext, CAIRO_OPERATOR_OVER);

	cairo_save (g_pMainContext);
	cairo_scale (g_pMainContext,
		     (double) iWidth / (double) g_DimensionData.width,
		     (double) iHeight / (double) g_DimensionData.height);
	cairo_translate (g_pMainContext, fHalfX, fHalfY);
	cairo_rotate (g_pMainContext, -M_PI/2.0f);

	if (g_iShowDate)
	{
		cairo_save (g_pMainContext);
		cairo_set_source_rgb (g_pMainContext, 1.0f, 0.5f, 0.0f);
		cairo_set_line_width (g_pMainContext, 5.0f);
		cairo_text_extents (g_pMainContext, g_acDate, &textExtents);
		cairo_rotate (g_pMainContext, (M_PI/180.0f) * 90.0f);
		cairo_move_to (g_pMainContext,
			       -textExtents.width / 2.0f,
			       2.0f * textExtents.height);
		cairo_show_text (g_pMainContext, g_acDate);
		cairo_restore (g_pMainContext);
	}

	cairo_save (g_pMainContext);
	cairo_translate (g_pMainContext, fShadowOffsetX, fShadowOffsetY);
	cairo_rotate (g_pMainContext, G_PI / 180.0f * fAngleHour);

	rsvg_handle_render_cairo (g_pSvgHandles[CLOCK_HOUR_HAND_SHADOW],
				  g_pMainContext);

	cairo_restore (g_pMainContext);

	cairo_save (g_pMainContext);
	cairo_translate (g_pMainContext, fShadowOffsetX, fShadowOffsetY);

	if (bAnimateMinute)
	{
		cairo_rotate (g_pMainContext,
			      G_PI / 180.0f * (fAngleMinute + fFactor * 6.0f));
		if (fFactor >= 0.95f)
		{
			bAnimateMinute = FALSE;
			fAngleMinute = (double) g_iMinutes * 6.0f;
		}
	}
	else
		cairo_rotate (g_pMainContext, G_PI / 180.0f * fAngleMinute);

	rsvg_handle_render_cairo (g_pSvgHandles[CLOCK_MINUTE_HAND_SHADOW],
				  g_pMainContext);

	cairo_restore (g_pMainContext);

	if (g_iShowSeconds)
	{
		cairo_save (g_pMainContext);
		cairo_translate (g_pMainContext, fShadowOffsetX, fShadowOffsetY);
		cairo_rotate (g_pMainContext,
			      G_PI / 180.0f *
			      (fAngleSecond + fFactor * 6.0f));

		rsvg_handle_render_cairo (g_pSvgHandles[CLOCK_SECOND_HAND_SHADOW],
					  g_pMainContext);

		cairo_restore (g_pMainContext);
	}

	cairo_save (g_pMainContext);
	cairo_rotate (g_pMainContext, G_PI / 180.0f * fAngleHour);

	rsvg_handle_render_cairo (g_pSvgHandles[CLOCK_HOUR_HAND],
				  g_pMainContext);

	cairo_restore (g_pMainContext);

	cairo_save (g_pMainContext);

	if (bAnimateMinute)
	{
		cairo_rotate (g_pMainContext,
			      G_PI / 180.0f * (fAngleMinute + fFactor * 6.0f));
		if (fFactor >= 0.95f)
		{
			bAnimateMinute = FALSE;
			fAngleMinute = (double) g_iMinutes * 6.0f;
		}
	}
	else
		cairo_rotate (g_pMainContext, G_PI / 180.0f * fAngleMinute);

	rsvg_handle_render_cairo (g_pSvgHandles[CLOCK_MINUTE_HAND],
				  g_pMainContext);

	cairo_restore (g_pMainContext);

	if (g_iShowSeconds)
	{
		cairo_save (g_pMainContext);
		cairo_rotate (g_pMainContext,
			      G_PI / 180.0f *
			      (fAngleSecond + fFactor * 6.0f));

		rsvg_handle_render_cairo (g_pSvgHandles[CLOCK_SECOND_HAND],
					  g_pMainContext);
		cairo_restore (g_pMainContext);
	}

	cairo_restore (g_pMainContext);

	cairo_set_source_surface (g_pMainContext,
				  g_pForegroundSurface,
				  0.0f,
				  0.0f);
	cairo_paint (g_pMainContext);

	/* get the current time-stamp */
	fCurrentTimeStamp = g_timer_elapsed (g_pTimer,
					     &ulMilliSeconds);
	ulMilliSeconds /= 1000;
	fFullSecond = fCurrentTimeStamp - fLastFullSecond;

	/* take care of the clock-hands anim-vars */
	if (fFullSecond < 1.0f)
		iFrames++;
	else
	{
		iFrames = 0;
		fLastFullSecond = fCurrentTimeStamp;

		fAngleSecond = (double) g_iSeconds * 6.0f;

		if (fAngleSecond == 360.0f)
			fAngleSecond = 0.0f;

		if (fAngleSecond == 354.0f)
			bAnimateMinute = TRUE;

		if (fAngleSecond == 360.0f)
			fAngleSecond = 0.0f;

		if (fAngleMinute == 360.0f)
			fAngleMinute = 0.0f;
	}
	fLastTimeStamp = fCurrentTimeStamp;
}

static void
on_startup_size_changed (GtkComboBox* pComboBox,
			 gpointer     window)
{
	switch (gtk_combo_box_get_active (pComboBox))
	{
		case SIZE_SMALL :
			gtk_widget_set_sensitive (g_pTableStartupSize, FALSE);
			gtk_window_resize (GTK_WINDOW (window), 50, 50);
		break;

		case SIZE_MEDIUM :
			gtk_widget_set_sensitive (g_pTableStartupSize, FALSE);
			gtk_window_resize (GTK_WINDOW (window), 100, 100);
		break;

		case SIZE_LARGE :
			gtk_widget_set_sensitive (g_pTableStartupSize, FALSE);
			gtk_window_resize (GTK_WINDOW (window), 200, 200);
		break;

		case SIZE_CUSTOM :
			gtk_widget_set_sensitive (g_pTableStartupSize, TRUE);
			gtk_window_resize (GTK_WINDOW (window),
					   gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (g_pSpinButtonWidth)),
					   gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (g_pSpinButtonHeight)));
		break;
	}

	g_bNeedsUpdate = TRUE;
}

static void
on_height_value_changed (GtkSpinButton*	pSpinButton,
			 gpointer	window)
{
	gint iWidth;
	gint iOldHeight;
	gint iNewHeight;

	g_bNeedsUpdate = TRUE;
	gtk_window_get_size (GTK_WINDOW (window), &iWidth, &iOldHeight);
	iNewHeight = gtk_spin_button_get_value_as_int (pSpinButton);

	if (is_power_of_two (iNewHeight))
	{
		iNewHeight +=1;
		gtk_spin_button_set_value (pSpinButton, (gdouble) iNewHeight);
	}

	if (iOldHeight != iNewHeight)
	{
		gtk_window_resize (GTK_WINDOW (window), iWidth, iNewHeight);
		gtk_widget_queue_draw (GTK_WIDGET (window));
	}
}

static void
on_width_value_changed (GtkSpinButton* pSpinButton,
			gpointer       window)
{
	gint iOldWidth;
	gint iNewWidth;
	gint iHeight;

	g_bNeedsUpdate = TRUE;
	gtk_window_get_size (GTK_WINDOW (window), &iOldWidth, &iHeight);
	iNewWidth = gtk_spin_button_get_value_as_int (pSpinButton);

	if (is_power_of_two (iNewWidth))
	{
		iNewWidth +=1;
		gtk_spin_button_set_value (pSpinButton, (gdouble) iNewWidth);
	}

	if (iOldWidth != iNewWidth)
	{
		gtk_window_resize (GTK_WINDOW (window), iNewWidth, iHeight);
		gtk_widget_queue_draw (GTK_WIDGET (window));
	}
}

static void
on_value_changed (GtkRange* pRange,
		  gpointer  data)
{
	g_iRefreshRate = (gint) gtk_range_get_value (pRange);
	g_source_remove (g_iuIntervalHandlerId);
	g_iuIntervalHandlerId = g_timeout_add (1000 / g_iRefreshRate,
					       (GSourceFunc) time_handler,
					       (gpointer) g_pMainWindow);
}

static void
on_seconds_toggled (GtkToggleButton* pTogglebutton,
		    gpointer	     window)
{
	if (gtk_toggle_button_get_active (pTogglebutton))
		g_iShowSeconds = 1;
	else
		g_iShowSeconds = 0;

	gtk_widget_queue_draw (GTK_WIDGET (window));
}

static void
on_date_toggled (GtkToggleButton* pTogglebutton,
		 gpointer	  window)
{
	if (gtk_toggle_button_get_active (pTogglebutton))
		g_iShowDate = 1;
	else
		g_iShowDate = 0;

	gtk_widget_queue_draw (GTK_WIDGET (window));
}

static void
on_keep_on_top_toggled (GtkToggleButton* pTogglebutton,
			gpointer	 window)
{
	if (gtk_toggle_button_get_active (pTogglebutton))
		g_iKeepOnTop = 1;
	else
		g_iKeepOnTop = 0;

	gtk_window_set_keep_above (GTK_WINDOW (window), g_iKeepOnTop);
}

static void
on_appear_in_pager_toggled (GtkToggleButton* pTogglebutton,
			    gpointer	     window)
{
	if (gtk_toggle_button_get_active (pTogglebutton))
		g_iAppearInPager = 1;
	else
		g_iAppearInPager = 0;

	gtk_window_set_skip_pager_hint (GTK_WINDOW (window), !g_iAppearInPager);
}

static void
on_appear_in_taskbar_toggled (GtkToggleButton* pTogglebutton,
			      gpointer	       window)
{
	if (gtk_toggle_button_get_active (pTogglebutton))
		g_iAppearInTaskbar = 1;
	else
		g_iAppearInTaskbar = 0;

	gtk_window_set_skip_taskbar_hint (GTK_WINDOW (window),
					  !g_iAppearInTaskbar);
}

static void
on_sticky_toggled (GtkToggleButton* pTogglebutton,
		   gpointer	    window)
{
	if (gtk_toggle_button_get_active (pTogglebutton))
	{
		g_iSticky = 1;
		gtk_window_stick (GTK_WINDOW (window));
	}
	else
	{
		g_iSticky = 0;
		gtk_window_unstick (GTK_WINDOW (window));
	}
}

static void
on_24h_toggled (GtkToggleButton* pTogglebutton,
		gpointer	 window)
{
	if (gtk_toggle_button_get_active (pTogglebutton))
		g_i24 = 1;
	else
		g_i24 = 0;
}

static void
on_help_clicked (GtkButton* pButton,
		 gpointer   data)
{
}

static gchar*
get_preferences_filename (void)
{
	gchar* pcFilename = NULL;
	gint iLength;

	iLength = strlen (g_get_home_dir ());
	iLength += strlen (".cairo-clockrc");
	iLength += 2;

	pcFilename = (gchar*) calloc (sizeof (gchar), iLength);
	if (!pcFilename)
		return NULL;

	strcpy (pcFilename, (gchar*) g_get_home_dir ());
	strcat (pcFilename, "/");
	strcat (pcFilename, ".cairo-clockrc");

	return pcFilename;
}

static gboolean
read_settings (gchar* pcFilePath,
	       gint*  piX,
	       gint*  piY,
	       gint*  piWidth,
	       gint*  piHeight,
	       gint*  piShowSeconds,
	       gint*  piShowDate,
	       gchar* pcTheme,
	       gint*  piKeepOnTop,
	       gint*  piAppearInPager,
	       gint*  piAppearInTaskbar,
	       gint*  piSticky,
	       gint*  pi24,
	       gint*  piRefreshRate)
{
	FILE* fileHandle = NULL;
	gint iResult = 0;

	if (!pcFilePath)
		return FALSE;

	fileHandle = fopen (pcFilePath, "r");
	if (!fileHandle)
		return FALSE;

	/* skip comment-line */
	while ((gchar)iResult != '\n')
		iResult = fgetc (fileHandle);

	iResult = fscanf (fileHandle, "x=%d\n", piX);
	iResult = fscanf (fileHandle, "y=%d\n", piY);
	iResult = fscanf (fileHandle, "width=%d\n", piWidth);
	iResult = fscanf (fileHandle, "height=%d\n", piHeight);
	iResult = fscanf (fileHandle, "show-seconds=%d\n", piShowSeconds);
	iResult = fscanf (fileHandle, "show-date=%d\n", piShowDate);
	iResult = fscanf (fileHandle, "theme=%s\n", pcTheme);
	iResult = fscanf (fileHandle, "keep-on-top=%d\n", piKeepOnTop);
	iResult = fscanf (fileHandle, "appear-in-pager=%d\n", piAppearInPager);
	iResult = fscanf (fileHandle, "appear-in-taskbar=%d\n", piAppearInTaskbar);
	iResult = fscanf (fileHandle, "sticky=%d\n", piSticky);
	iResult = fscanf (fileHandle, "twentyfour=%d\n", pi24);
	iResult = fscanf (fileHandle, "refreshrate=%d\n", piRefreshRate);

	iResult = fclose (fileHandle);

	return TRUE;
}

static gboolean
write_settings (gchar* pcFilePath,
		gint   iX,
		gint   iY,
		gint   iWidth,
		gint   iHeight,
		gint   iShowSeconds,
		gint   iShowDate,
		gchar* pcTheme,
		gint   iKeepOnTop,
		gint   iAppearInPager,
		gint   iAppearInTaskbar,
		gint   iSticky,
		gint   i24,
		gint   iRefreshRate)
{
	FILE* fileHandle = NULL;

	if (!pcFilePath)
		return FALSE;

	fileHandle = fopen (pcFilePath, "w");
	if (!fileHandle)
		return FALSE;

	fprintf (fileHandle,
		 "%s %s %s\n", _("# This file is machine-generated."),
		 _("Do not edit it manually!"),
		 _("I really mean it!!!"));
	fprintf (fileHandle, "x=%d\n", iX);
	fprintf (fileHandle, "y=%d\n", iY);
	fprintf (fileHandle, "width=%d\n", iWidth);
	fprintf (fileHandle, "height=%d\n", iHeight);
	fprintf (fileHandle, "show-seconds=%d\n", iShowSeconds);
	fprintf (fileHandle, "show-date=%d\n", iShowDate);
	fprintf (fileHandle, "theme=%s\n", pcTheme);
	fprintf (fileHandle, "keep-on-top=%d\n", iKeepOnTop);
	fprintf (fileHandle, "appear-in-pager=%d\n", iAppearInPager);
	fprintf (fileHandle, "appear-in-taskbar=%d\n", iAppearInTaskbar);
	fprintf (fileHandle, "sticky=%d\n", iSticky);
	fprintf (fileHandle, "twentyfour=%d\n", i24);
	fprintf (fileHandle, "refreshrate=%d\n", iRefreshRate);

	fclose (fileHandle);

	return TRUE;
}

static void
on_close_clicked (GtkButton* pButton,
		  gpointer   data)
{
	gchar* pcFilename = NULL;

	gtk_widget_hide (g_pSettingsDialog);

	pcFilename = get_preferences_filename ();
	if (!write_settings (pcFilename,
			     g_iDefaultX,
			     g_iDefaultY,
			     g_iDefaultWidth,
			     g_iDefaultHeight,
			     g_iShowSeconds,
			     g_iShowDate,
			     g_acTheme,
			     g_iKeepOnTop,
			     g_iAppearInPager,
			     g_iAppearInTaskbar,
			     g_iSticky,
			     g_i24,
			     g_iRefreshRate))
		printf (_("Ups, there was an error while trying to save the preferences!\n"));

	if (pcFilename)
		free (pcFilename);
}

static GList*
get_theme_list (GString* pSystemPath,
		GString* pUserPath)
{
	GDir*	    pThemeDir	= NULL;
	GString*    pThemeName	= NULL;
	GList*	    pThemeList	= NULL;
	ThemeEntry* pThemeEntry	= NULL;

	pThemeDir = g_dir_open (pSystemPath->str, 0, NULL);
	if (!pThemeDir)
		return NULL;

	pThemeName = g_string_new (g_dir_read_name (pThemeDir));
	while (pThemeName->len != 0)
	{
		pThemeEntry = (ThemeEntry*) g_malloc0 (sizeof (ThemeEntry));
		if (pThemeName && pThemeEntry)
		{
			pThemeEntry->pPath = g_string_new (pSystemPath->str);
			pThemeEntry->pName = g_string_new (pThemeName->str);
			pThemeList = g_list_append (pThemeList,
						    (gpointer) pThemeEntry);
		}
		else
		{
			g_free (pThemeEntry);
			g_string_free (pThemeName, TRUE);
		}

		pThemeName = g_string_new (g_dir_read_name (pThemeDir));
	}
	g_dir_close (pThemeDir);

	pThemeDir = g_dir_open (pUserPath->str, 0, NULL);
	if (!pThemeDir)
		return pThemeList;

	pThemeName = g_string_new (g_dir_read_name (pThemeDir));
	while (pThemeName->len != 0)
	{
		pThemeEntry = (ThemeEntry*) g_malloc0 (sizeof (ThemeEntry));
		if (pThemeName && pThemeEntry)
		{
			pThemeEntry->pPath = g_string_new (pUserPath->str);
			pThemeEntry->pName = g_string_new (pThemeName->str);
			pThemeList = g_list_append (pThemeList,
						    (gpointer) pThemeEntry);
		}
		else
		{
			g_free (pThemeEntry);
			g_string_free (pThemeName, TRUE);
		}

		pThemeName = g_string_new (g_dir_read_name (pThemeDir));
	}
	g_dir_close (pThemeDir);

	return g_list_first (pThemeList);
}

static void
delete_entry (gpointer themeEntry, gpointer data)
{
	g_string_free (((ThemeEntry*) themeEntry)->pPath, TRUE);
	g_string_free (((ThemeEntry*) themeEntry)->pName, TRUE);
	g_free (themeEntry);
}

static void
theme_list_delete (GList* pThemeList)
{
	g_list_foreach (pThemeList, delete_entry, NULL);
}

static void
change_theme (GList*	 pThemeList,
	      guint	 uiThemeIndex,
	      GtkWidget* pWindow)
{
	GList*	pThemeEntry    = NULL;
	gchar*	pcFullFilename = NULL;
	gint	iElement       = 0;
	GError*	pError	       = NULL;

	if (!pThemeList)
		return;

	pThemeEntry = g_list_nth (pThemeList, uiThemeIndex);

	if (!pThemeEntry)
		return;

	if (pWindow)
		for (iElement = 0; iElement < CLOCK_ELEMENTS; iElement++)
			rsvg_handle_free (g_pSvgHandles[iElement]);

	for (iElement = 0; iElement < CLOCK_ELEMENTS; iElement++)
	{
		pcFullFilename = (gchar*) calloc (sizeof (gchar*),
						  ((ThemeEntry*) (pThemeEntry->data))->pPath->len +
						  ((ThemeEntry*) (pThemeEntry->data))->pName->len +
						  strlen (g_cFileNames[iElement])
						  + 3);
		strcpy (pcFullFilename, ((ThemeEntry*) (pThemeEntry->data))->pPath->str);
		strcat (pcFullFilename, "/");
		strcat (pcFullFilename, ((ThemeEntry*) (pThemeEntry->data))->pName->str);
		strcat (pcFullFilename, "/");
		strcat (pcFullFilename, g_cFileNames[iElement]);
		g_pSvgHandles[iElement] = rsvg_handle_new_from_file (pcFullFilename,
								     &pError);

		free (pcFullFilename);
	}

	/* update size-info of loaded theme */
	rsvg_handle_get_dimensions (g_pSvgHandles[CLOCK_DROP_SHADOW],
				    &g_DimensionData);

	if (pWindow)
		gtk_widget_queue_draw (pWindow);
}

static void
print_theme_entry (gpointer themeEntry,
		   gpointer data)
{
	printf ("%s (%s)\n",
		((ThemeEntry*) themeEntry)->pName->str,
		((ThemeEntry*) themeEntry)->pPath->str);
}

static void
print_theme_list (GList* pThemeList)
{
	g_list_foreach (pThemeList, print_theme_entry, NULL);
}

static void
on_theme_changed (GtkComboBox* pComboBox,
		  gpointer     data)
{
	g_bNeedsUpdate = TRUE;
	sprintf (g_acTheme, "%s", gtk_combo_box_get_active_text (pComboBox));
	change_theme (g_pThemeList,
		      gtk_combo_box_get_active (pComboBox),
		      GTK_WIDGET (data));
	update_input_shape (GTK_WIDGET (data),
			    g_iDefaultWidth,
			    g_iDefaultHeight);
}

static gboolean
on_configure (GtkWidget*	 pWidget,
	      GdkEventConfigure* pEvent,
	      gpointer		 data)
{
	gint iNewWidth	= pEvent->width;
	gint iNewHeight	= pEvent->height;

	if (iNewWidth != g_iDefaultWidth || iNewHeight != g_iDefaultHeight)
	{
		g_bNeedsUpdate = TRUE;
		update_input_shape (pWidget, iNewWidth, iNewHeight);
		g_iDefaultWidth = iNewWidth;
		g_iDefaultHeight = iNewHeight;
	}

	return FALSE;
}

static gchar*
get_system_theme_path (void)
{
	return PKGDATA_DIR "/themes";
}

static gchar*
get_user_theme_path (void)
{
	gchar* pcFilename = NULL;
	gint iLength;

	iLength = strlen (g_get_home_dir ());
	iLength += strlen (".cairo-clock/themes");
	iLength += 2;

	pcFilename = (gchar*) calloc (sizeof (gchar), iLength);
	if (!pcFilename)
		return NULL;

	strcpy (pcFilename, (gchar*) g_get_home_dir ());
	strcat (pcFilename, "/");
	strcat (pcFilename, ".cairo-clock/themes");

	return pcFilename;
}

static gchar*
get_glade_filename (void)
{
	return PKGDATA_DIR "/glade/cairo-clock.glade";
}

static gchar*
get_icon_filename (void)
{
	return DATA_DIR "/pixmaps/cairo-clock.png";
}

static gchar*
get_logo_filename (void)
{
	return PKGDATA_DIR "/pixmaps/cairo-clock-logo.png";
}

#if !GTK_CHECK_VERSION(2,9,0)
/* this is piece by piece taken from gtk+ 2.9.0 (CVS-head with a patch applied
regarding XShape's input-masks) so people without gtk+ >= 2.9.0 can compile and
run input_shape_test.c */
static void
do_shape_combine_mask (GdkWindow* pWindow,
		       GdkBitmap* pMask,
		       gint	  iX,
		       gint	  iY)
{
	Pixmap pixmap;
	gint   iIgnore;
	gint   iMajor;
	gint   iMinor;

	if (!XShapeQueryExtension (GDK_WINDOW_XDISPLAY (pWindow),
				   &iIgnore,
				   &iIgnore))
		return;

	if (!XShapeQueryVersion (GDK_WINDOW_XDISPLAY (pWindow),
				 &iMajor,
				 &iMinor))
		return;

	/* for shaped input we need at least XShape 1.1 */
	if (iMajor != 1 && iMinor < 1)
		return;

	if (pMask)
		pixmap = GDK_DRAWABLE_XID (pMask);
	else
	{
		iX = 0;
		iY = 0;
		pixmap = None;
	}

	XShapeCombineMask (GDK_WINDOW_XDISPLAY (pWindow),
			   GDK_DRAWABLE_XID (pWindow),
			   ShapeInput,
			   iX,
			   iY,
			   pixmap,
			   ShapeSet);
}
#endif

static void
update_input_shape (GtkWidget* pWindow,
		    gint       iWidth,
		    gint       iHeight)
{
	GdkBitmap* pShapeBitmap = NULL;
	cairo_t*   pCairoContext = NULL;

	pShapeBitmap = (GdkBitmap*) gdk_pixmap_new (NULL, iWidth, iHeight, 1);
	if (pShapeBitmap)
	{
		pCairoContext = gdk_cairo_create (pShapeBitmap);
		if (cairo_status (pCairoContext) == CAIRO_STATUS_SUCCESS)
		{
			/* use drop-shadow, clock-face and marks as "clickable" areas */
			draw_background (pCairoContext, iWidth, iHeight);
			cairo_destroy (pCairoContext);
#if !GTK_CHECK_VERSION(2,9,0)
			do_shape_combine_mask (pWindow->window, NULL, 0, 0);
			do_shape_combine_mask (pWindow->window,
					       pShapeBitmap,
					       0,
					       0);
#else
			gtk_widget_input_shape_combine_mask (pWindow,
							     NULL,
							     0,
							     0);
			gtk_widget_input_shape_combine_mask (pWindow,
							     pShapeBitmap,
							     0,
							     0);
#endif
		}
		g_object_unref ((gpointer) pShapeBitmap);
	}
}

int
main (int    argc,
      char** argv)
{
	GdkGeometry	     hints;
	gint		     iElement;
	GladeXML*	     pGladeXml;
	GtkWidget*	     pSettingsMenuItem		 = NULL;
	GtkWidget*	     pInfoMenuItem		 = NULL;
	GtkWidget*	     pQuitMenuItem		 = NULL;
	GtkWidget*	     pComboBoxTheme		 = NULL;
	GtkWidget*	     pCheckButtonSeconds	 = NULL;
	GtkWidget*	     pCheckButtonDate		 = NULL;
	GtkWidget*	     pCheckButtonKeepOnTop	 = NULL;
	GtkWidget*	     pCheckButtonAppearInPager	 = NULL;
	GtkWidget*	     pCheckButtonAppearInTaskbar = NULL;
	GtkWidget*	     pCheckButtonSticky		 = NULL;
	GtkWidget*	     pCheckButton24h		 = NULL;
	GtkWidget*	     pButtonHelp		 = NULL;
	GtkWidget*	     pButtonClose		 = NULL;
	GList*		     pThemeEntry		 = NULL;
	gchar*		     pcFilename			 = NULL;
	GError*		     pError			 = NULL;
	GOptionContext*	     pOptionContext		 = NULL;
	GOptionEntry	     aOptions[]	= {{"xposition",
					    'x',
					    0,
					    G_OPTION_ARG_INT,
					    &g_iDefaultX,
					    _("x-position of the top-left window-corner"),
					    "X"},
					   {"yposition",
					    'y',
					    0,
					    G_OPTION_ARG_INT,
					    &g_iDefaultY,
					    _("y-position of the top-left window-corner"),
					    "Y"},
					   {"width",
					    'w',
					    0,
					    G_OPTION_ARG_INT,
					    &g_iDefaultWidth,
					    _("open window with this width"),
					    "WIDTH"},
					   {"height",
					    'h',
					    0,
					    G_OPTION_ARG_INT,
					    &g_iDefaultHeight,
					    _("open window with this height"),
					    "HEIGHT"},
					   {"seconds",
					    's',
					    0,
					    G_OPTION_ARG_NONE,
					    &g_iShowSeconds,
					    _("draw seconds hand"),
					    NULL},
					   {"date",
					    'd',
					    0,
					    G_OPTION_ARG_NONE,
					    &g_iShowDate,
					    _("draw date-display"),
					    NULL},
					   {"list",
					    'l',
					    0,
					    G_OPTION_ARG_NONE,
					    &bPrintThemeList,
					    _("list installed themes and exit"),
					    NULL},
					   {"theme",
					    't',
					    0,
					    G_OPTION_ARG_STRING,
					    &g_pcTheme,
					    _("theme to draw the clock with"),
					    "NAME"},
					   {"ontop",
					    'o',
					    0,
					    G_OPTION_ARG_NONE,
					    &g_iKeepOnTop,
					    _("clock-window stays on top of all windows"),
					    NULL},
					   {"pager",
					    'p',
					    0,
					    G_OPTION_ARG_NONE,
					    &g_iAppearInPager,
					    _("clock-window shows up in pager"),
					    NULL},
					   {"taskbar",
					    'b',
					    0,
					    G_OPTION_ARG_NONE,
					    &g_iAppearInTaskbar,
					    _("clock-window shows up in taskbar"),
					    NULL},
					   {"sticky",
					    'i',
					    0,
					    G_OPTION_ARG_NONE,
					    &g_iSticky,
					    _("clock-window sticks to all workspaces"),
					    NULL},
					   {"twelve",
					    'e',
					    0,
					    G_OPTION_ARG_NONE,
					    &g_i12,
					    _("hands work in 12 hour mode"),
					    NULL},
					   {"twentyfour",
					    'f',
					    0,
					    G_OPTION_ARG_NONE,
					    &g_i24,
					    _("hands work in 24 hour mode"),
					    NULL},
					   {"refresh",
					    'r',
					    0,
					    G_OPTION_ARG_INT,
					    &g_iRefreshRate,
					    _("render at RATE (default: 30 Hz)"),
					    "RATE"},
					   {"version",
					    'v',
					    0,
					    G_OPTION_ARG_NONE,
					    &bPrintVersion,
					    _("print version of program and exit"),
					    NULL},
					   {NULL}};

	bindtextdomain (GETTEXT_PACKAGE, CAIROCLOCKLOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	/* read in names of all installed themes */
	g_pThemeList = get_theme_list (g_string_new (get_system_theme_path ()),
				       g_string_new (get_user_theme_path ()));

	gtk_init (&argc, &argv);

	if (argc > 1)
	{
		/* setup and process command-line options */
		pOptionContext = g_option_context_new (_("- analog clock drawn with vector-graphics"));
		g_option_context_add_main_entries (pOptionContext,
						   aOptions,
						   "cairo-clock");
		g_option_context_parse (pOptionContext, &argc, &argv, NULL);
		g_option_context_free (pOptionContext);

		if (g_pcTheme)
			strcpy (g_acTheme, g_pcTheme);

		if (g_iDefaultX <= 0 ||
		    g_iDefaultX >= gdk_screen_get_width (gdk_screen_get_default ()))
			g_iDefaultX = 0;

		if (g_iDefaultY <= 0 ||
		    g_iDefaultY >= gdk_screen_get_height (gdk_screen_get_default ()))
			g_iDefaultY = 0;

		if (g_iRefreshRate <= MIN_REFRESH_RATE ||
		    g_iRefreshRate >= MAX_REFRESH_RATE)
			g_iRefreshRate = 30;
	}
	else
	{
		pcFilename = get_preferences_filename ();
		read_settings (pcFilename,
			       &g_iDefaultX,
			       &g_iDefaultY,
			       &g_iDefaultWidth,
			       &g_iDefaultHeight,
			       &g_iShowSeconds,
			       &g_iShowDate,
			       g_acTheme,
			       &g_iKeepOnTop,
			       &g_iAppearInPager,
			       &g_iAppearInTaskbar,
			       &g_iSticky,
			       &g_i24,
			       &g_iRefreshRate);
		if (pcFilename)
			free (pcFilename);
	}

	if (g_iDefaultWidth <= MIN_WIDTH ||
	    g_iDefaultWidth >= MAX_WIDTH)
		g_iDefaultWidth = 100;

	if (is_power_of_two (g_iDefaultWidth))
		g_iDefaultWidth += 1;

	if (g_iDefaultHeight <= MIN_HEIGHT ||
	    g_iDefaultHeight >= MAX_HEIGHT)
		g_iDefaultHeight = 100;

	if (is_power_of_two (g_iDefaultHeight))
		g_iDefaultHeight += 1;

	if (bPrintVersion)
	{
		printf ("%s %s\n", g_acAppName, g_acAppVersion);
		exit (0);
	}

	if (bPrintThemeList)
	{
		print_theme_list (g_pThemeList);
		exit (0);
	}

	rsvg_init ();
	pGladeXml = glade_xml_new (get_glade_filename (), NULL, NULL);
	if (!pGladeXml)
	{
		printf (_("Could not load \"%s\"!\n"), get_glade_filename ());
		exit (1);
	}

	g_pMainWindow = glade_xml_get_widget (pGladeXml,
					      "mainWindow");
	g_pErrorDialog = glade_xml_get_widget (pGladeXml,
					      "errorDialog");

	if (!gdk_screen_is_composited (gtk_widget_get_screen (g_pMainWindow)))
	{
		gtk_window_set_icon_from_file (GTK_WINDOW (g_pErrorDialog),
					       get_icon_filename (),
					       NULL);
		gtk_dialog_run (GTK_DIALOG (g_pErrorDialog));
		exit (2);
	}

	g_pPopUpMenu = glade_xml_get_widget (pGladeXml,
					     "popUpMenu");
	pSettingsMenuItem = glade_xml_get_widget (pGladeXml,
						  "settingsMenuItem");
	pInfoMenuItem = glade_xml_get_widget (pGladeXml,
					      "infoMenuItem");
	pQuitMenuItem = glade_xml_get_widget (pGladeXml,
					      "quitMenuItem");
	g_pSettingsDialog = glade_xml_get_widget (pGladeXml,
						  "settingsDialog");
	gtk_window_set_icon_from_file (GTK_WINDOW (g_pSettingsDialog),
				       get_icon_filename (),
				       NULL);
	g_pInfoDialog = glade_xml_get_widget (pGladeXml,
					      "infoDialog");
	gtk_window_set_icon_from_file (GTK_WINDOW (g_pInfoDialog),
				       get_icon_filename (),
				       NULL);
	gtk_about_dialog_set_version (GTK_ABOUT_DIALOG (g_pInfoDialog),
				      g_acAppVersion);
	gtk_about_dialog_set_logo (GTK_ABOUT_DIALOG (g_pInfoDialog),
				   gdk_pixbuf_new_from_file (get_logo_filename (),
				   &pError));
	g_pComboBoxStartupSize = glade_xml_get_widget (pGladeXml,
						       "comboboxStartupSize");
	g_pTableStartupSize = glade_xml_get_widget (pGladeXml,
						    "tableStartupSize");
	g_pSpinButtonWidth = glade_xml_get_widget (pGladeXml,
						   "spinbuttonWidth");
	g_pSpinButtonHeight = glade_xml_get_widget (pGladeXml,
						    "spinbuttonHeight");
	g_pHScaleSmoothness = glade_xml_get_widget (pGladeXml,
						    "hscaleSmoothness");
	pComboBoxTheme = glade_xml_get_widget (pGladeXml,
					       "comboboxTheme");
	pCheckButtonSeconds = glade_xml_get_widget (pGladeXml,
						    "checkbuttonSeconds");
	pCheckButtonDate = glade_xml_get_widget (pGladeXml,
						 "checkbuttonDate");
	pCheckButtonKeepOnTop = glade_xml_get_widget (pGladeXml,
						      "checkbuttonKeepOnTop");
	pCheckButtonAppearInPager = glade_xml_get_widget (pGladeXml,
							  "checkbuttonAppearInPager");
	pCheckButtonAppearInTaskbar = glade_xml_get_widget (pGladeXml,
							    "checkbuttonAppearInTaskbar");
	pCheckButtonSticky = glade_xml_get_widget (pGladeXml,
						   "checkbuttonSticky");
	pCheckButton24h = glade_xml_get_widget (pGladeXml,
						"checkbutton24h");
	pButtonHelp = glade_xml_get_widget (pGladeXml,
					    "buttonHelp");
	pButtonClose = glade_xml_get_widget (pGladeXml,
					     "buttonClose");

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pCheckButtonSeconds),
				      g_iShowSeconds);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pCheckButtonDate),
				      g_iShowDate);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pCheckButtonKeepOnTop),
				      g_iKeepOnTop);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pCheckButtonAppearInPager),
				      g_iAppearInPager);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pCheckButtonAppearInTaskbar),
				      g_iAppearInTaskbar);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pCheckButtonSticky),
				      g_iSticky);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pCheckButton24h),
				      g_i24);

	pThemeEntry = g_list_first (g_pThemeList);
	while (pThemeEntry)
	{
		gtk_combo_box_append_text (GTK_COMBO_BOX (pComboBoxTheme),
					   ((ThemeEntry*) (pThemeEntry->data))->pName->str);
		if (!strcmp (g_acTheme, ((ThemeEntry*) (pThemeEntry->data))->pName->str))
		{
			gtk_combo_box_set_active (GTK_COMBO_BOX (pComboBoxTheme),
						  g_iThemeCounter);
			change_theme (g_pThemeList, g_iThemeCounter, NULL);
		}
		g_iThemeCounter++;
		pThemeEntry = g_list_next (pThemeEntry);
	}

	rsvg_handle_get_dimensions (g_pSvgHandles[CLOCK_DROP_SHADOW],
				    &g_DimensionData);

	gtk_window_set_decorated (GTK_WINDOW (g_pMainWindow), FALSE);
	gtk_window_set_resizable (GTK_WINDOW (g_pMainWindow), TRUE);
	gtk_widget_set_app_paintable (g_pMainWindow, TRUE);
	gtk_window_set_icon_from_file (GTK_WINDOW (g_pMainWindow),
				       get_icon_filename (),
				       NULL);
	gtk_window_set_title (GTK_WINDOW (g_pMainWindow),
			      _("MacSlow's Cairo-Clock"));
	gtk_window_set_default_size (GTK_WINDOW (g_pMainWindow),
				     g_iDefaultWidth,
				     g_iDefaultHeight);
	gtk_window_set_keep_above (GTK_WINDOW (g_pMainWindow), g_iKeepOnTop);
	gtk_window_set_skip_pager_hint (GTK_WINDOW (g_pMainWindow),
					!g_iAppearInPager);
	gtk_window_set_skip_taskbar_hint (GTK_WINDOW (g_pMainWindow),
					  !g_iAppearInTaskbar);
	if (g_iSticky)
		gtk_window_stick (GTK_WINDOW (g_pMainWindow));
	else
		gtk_window_unstick (GTK_WINDOW (g_pMainWindow));

	gtk_spin_button_set_value (GTK_SPIN_BUTTON (g_pSpinButtonWidth),
				   (gdouble) g_iDefaultWidth);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (g_pSpinButtonHeight),
				   (gdouble) g_iDefaultHeight);
	gtk_range_set_value (GTK_RANGE (g_pHScaleSmoothness), g_iRefreshRate);
	gtk_combo_box_set_active (GTK_COMBO_BOX (g_pComboBoxStartupSize),
				  SIZE_CUSTOM);
	gtk_widget_set_sensitive (g_pTableStartupSize, TRUE);

	if (g_iDefaultX <= gdk_screen_width () &&
	    g_iDefaultX >= 0 &&
	    g_iDefaultY <= gdk_screen_height () &&
	    g_iDefaultY >= 0)
		gtk_window_move (GTK_WINDOW (g_pMainWindow),
				 g_iDefaultX,
				 g_iDefaultY);

	hints.min_width	 = MIN_WIDTH;
	hints.min_height = MIN_HEIGHT;
	hints.max_width	 = MAX_WIDTH;
	hints.max_height = MAX_HEIGHT;

	gtk_window_set_geometry_hints (GTK_WINDOW (g_pMainWindow),
				       g_pMainWindow,
				       &hints,
				       GDK_HINT_MIN_SIZE |
				       GDK_HINT_MAX_SIZE);

	on_alpha_screen_changed (g_pMainWindow, NULL, NULL);

	/* that's needed here because a top-level GtkWindow does not listen to
	 * "button-press-events" by default */
	gtk_widget_add_events (g_pMainWindow, GDK_BUTTON_PRESS_MASK);

	g_signal_connect (G_OBJECT (g_pMainWindow),
			  "expose-event",
			  G_CALLBACK (on_expose),
			  NULL);
	g_signal_connect (G_OBJECT (g_pMainWindow),
			  "configure-event",
			  G_CALLBACK (on_configure),
			  NULL);
	g_signal_connect (G_OBJECT (g_pMainWindow),
			  "key-press-event",
			  G_CALLBACK (on_key_press),
			  NULL);
	g_signal_connect (G_OBJECT (g_pMainWindow),
			  "button-press-event",
			  G_CALLBACK (on_button_press),
			  NULL);
	g_signal_connect (G_OBJECT (pSettingsMenuItem),
			  "activate",
			  G_CALLBACK (on_settings_activate),
			  NULL);
	g_signal_connect (G_OBJECT (pInfoMenuItem),
			  "activate",
			  G_CALLBACK (on_info_activate),
			  NULL);
	g_signal_connect (G_OBJECT (pQuitMenuItem),
			  "activate",
			  G_CALLBACK (on_quit_activate),
			  NULL);
	g_signal_connect (G_OBJECT (g_pInfoDialog),
			  "response",
			  G_CALLBACK (on_info_close),
			  NULL);
	g_signal_connect (G_OBJECT (g_pComboBoxStartupSize),
			  "changed",
			  G_CALLBACK (on_startup_size_changed),
			  g_pMainWindow);
	g_signal_connect (G_OBJECT (g_pSpinButtonWidth),
			  "value-changed",
			  G_CALLBACK (on_width_value_changed),
			  g_pMainWindow);
	g_signal_connect (G_OBJECT (g_pSpinButtonHeight),
			  "value-changed",
			  G_CALLBACK (on_height_value_changed),
			  g_pMainWindow);
	g_signal_connect (G_OBJECT (g_pHScaleSmoothness),
			  "value-changed",
			  G_CALLBACK (on_value_changed),
			  NULL);
	g_signal_connect (G_OBJECT (pComboBoxTheme),
			  "changed",
			  G_CALLBACK (on_theme_changed),
			  (gpointer) g_pMainWindow);
	g_signal_connect (G_OBJECT (pCheckButtonSeconds),
			  "toggled",
			  G_CALLBACK (on_seconds_toggled),
			  g_pMainWindow);
	g_signal_connect (G_OBJECT (pCheckButtonDate),
			  "toggled",
			  G_CALLBACK (on_date_toggled),
			  g_pMainWindow);
	g_signal_connect (G_OBJECT (pCheckButtonKeepOnTop),
			  "toggled",
			  G_CALLBACK (on_keep_on_top_toggled),
			  g_pMainWindow);
	g_signal_connect (G_OBJECT (pCheckButtonAppearInPager),
			  "toggled",
			  G_CALLBACK (on_appear_in_pager_toggled),
			  g_pMainWindow);
	g_signal_connect (G_OBJECT (pCheckButtonAppearInTaskbar),
			  "toggled",
			  G_CALLBACK (on_appear_in_taskbar_toggled),
			  g_pMainWindow);
	g_signal_connect (G_OBJECT (pCheckButtonSticky),
			  "toggled",
			  G_CALLBACK (on_sticky_toggled),
			  g_pMainWindow);
	g_signal_connect (G_OBJECT (pCheckButton24h),
			  "toggled",
			  G_CALLBACK (on_24h_toggled),
			  g_pMainWindow);
	g_signal_connect (G_OBJECT (pButtonHelp),
			  "clicked",
			  G_CALLBACK (on_help_clicked),
			  NULL);
	g_signal_connect (G_OBJECT (pButtonClose),
			  "clicked",
			  G_CALLBACK (on_close_clicked),
			  NULL);

	if (!GTK_WIDGET_VISIBLE (g_pMainWindow))
	{
		gtk_widget_realize (g_pMainWindow);
		gdk_window_set_back_pixmap (g_pMainWindow->window, NULL, FALSE);
		gtk_widget_show_all (g_pMainWindow);
	}

	g_iuIntervalHandlerId = g_timeout_add (1000 / g_iRefreshRate,
					       (GSourceFunc) time_handler,
					       (gpointer) g_pMainWindow);

	g_pMainContext = gdk_cairo_create (g_pMainWindow->window);

	update_input_shape (g_pMainWindow, g_iDefaultWidth, g_iDefaultHeight);
	g_pTimer = g_timer_new ();

	gtk_main ();

	for (iElement = 0; iElement < CLOCK_ELEMENTS; iElement++)
		rsvg_handle_free (g_pSvgHandles[iElement]);

	rsvg_term ();

	if (g_pThemeList)
		theme_list_delete (g_pThemeList);

	gtk_window_get_position (GTK_WINDOW (g_pMainWindow),
				 &g_iDefaultX,
				 &g_iDefaultY);
	gtk_window_get_size (GTK_WINDOW (g_pMainWindow),
			     &g_iDefaultWidth,
			     &g_iDefaultHeight);

	pcFilename = get_preferences_filename ();
	if (!write_settings (pcFilename,
			     g_iDefaultX,
			     g_iDefaultY,
			     g_iDefaultWidth,
			     g_iDefaultHeight,
			     g_iShowSeconds,
			     g_iShowDate,
			     g_acTheme,
			     g_iKeepOnTop,
			     g_iAppearInPager,
			     g_iAppearInTaskbar,
			     g_iSticky,
			     g_i24,
			     g_iRefreshRate))
		printf (_("Ups, error while trying to save the preferences!\n"));

	if (pcFilename)
		free (pcFilename);

	g_timer_destroy (g_pTimer);
	
	return 0;
}
