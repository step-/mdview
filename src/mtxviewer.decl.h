gboolean
mtx_viewer_present_page (MtxViewer *mvr,
                         const gchar *page,
                         guint offset);

void
mtx_viewer_destroy (MtxViewer *mvr);

MtxViewer *
mtx_viewer_new (const gchar *base_dir,
                const gchar *base_file,
                const gchar *title,
                GtkWindow *parent,
                const MtxViewerOptions *options);

/***********************************************************/

static void
_nav_trail_print_status_bar (MtxViewer *mvr);

static void
_nav_trail_back (MtxViewer *mvr);

static void
_nav_trail_fore (MtxViewer *mvr);

static void
_nav_trail_insert (MtxViewer *mvr,
                   const gchar *file,
                   guint offset);

static void
_nav_trail_fore_clear (MtxViewer *mvr);

static void
_scroll_to_curpos (MtxViewer *mvr,
                   const gint curpos,
                   const MtxTextViewHilightMode highlight);

static gboolean
idle_scroll_to_current_curpos (MtxViewer *mvr);

static void
open_url (MtxViewer *mvr,
          const gchar *url);

static void
edit_text_file (MtxViewer *mvr,
                const gchar *file);

static gboolean
mtx_viewer_save_backing_file (MtxViewer *mvr,
                              const gchar *buf,
                              const gsize size);

static gboolean
progress_logger_start (MtxViewer *mvr);

static void
progress_logger_stop (MtxViewer *mvr);

static void
progress_logger_schedule (MtxViewer *mvr);

static gboolean
progress_logger_update (progress_logger_update_data *data);

static void
progress_logger_thread_cb (GTask *task,
                           gpointer source_object,
                           gpointer task_data,
                           GCancellable *cancellable);

static void
progress_logger_async (GObject *object,
                       MtxViewer *mvr,
                       GCancellable *cancellable,
                       GAsyncReadyCallback callback,
                       gpointer user_data);

static gint
progress_logger_finish (GAsyncResult *result,
                        GError **error);

static void
progress_logger_completed (GObject *object,
                           GAsyncResult *result,
                           gpointer user_data);

static gboolean
statusbar_warn_pop (gpointer data);

static void
mtx_viewer_statusbar_warn_seconds (MtxViewer *mvr,
                                   const guint seconds,
                                   const gchar *message);

static gboolean
mtx_viewer_is_page_in_progress (MtxViewer *mvr);

static void
nav_fore_cb (gpointer *instance,
             gboolean cond,
             GError *error,    /*owned */
             gpointer data[]);

static void
nav_back_cb (gpointer *instance,
             gboolean cond,
             GError *error,    /*owned */
             gpointer data[]);

static void
nav_home_clicked_cb (gpointer *instance,
                     gboolean cond,
                     GError *error,    /*owned */
                     gpointer data[]);

static void
on_link_clicked_cb (gpointer *instance,
                    gboolean cond,
                    GError *error,    /*owned */
                    gpointer data[]);

static void
search_entry_activate_cb (gpointer *instance,
                          gboolean cond,
                          GError *error,    /*owned */
                          gpointer data[]);

static void
do_insert_page_cb (gpointer *instance,
                   gboolean cond,
                   GError *error,    /*owned */
                   gpointer data[]);

static void
present_page_cb (gpointer *instance,
                 gboolean cond,
                 GError *error,    /* owned */
                 gpointer data[]);

static void
error_page_cb (gpointer *instance,
               gboolean cond,
               GError *error,    /* owned */
               gpointer data[]);

static void
mtx_viewer_write_toc_to_backing_file (MtxViewer *mvr);

static void
mtx_viewer_insert_error_page (MtxViewer *mvr,
                              const gchar *page,
                              GError *error);

static gboolean
mtx_viewer_route_page (MtxViewer *mvr,
                       const gchar *path,
                       GClosure *completer);

static void
nav_fore_clicked (GtkWidget *widget,
                  gpointer data);

static gboolean
accel_nav_fore (GtkAccelGroup *group,
                GObject *obj,
                guint *keyval,
                GdkModifierType mod,
                gpointer data);

static void
nav_back_clicked (GtkWidget *widget,
                  gpointer data);

static gboolean
accel_nav_back (GtkAccelGroup *group,
                GObject * obj,
                guint keyval,
                GdkModifierType mod,
                gpointer data);

static gboolean
link_info_dest_equal (gconstpointer *a,
                      gconstpointer *b);

static void
on_link_clicked (MtxTextView *text_view,
                 const gchar *link_dest,
                 gpointer data);

static void
cancel_loading_clicked (GtkWidget *widget,
                        gpointer data);

static gboolean
accel_cancel_loading (GtkAccelGroup *group,
                      GObject *obj,
                      guint keyval,
                      GdkModifierType mod,
                      gpointer data);

static void
preview_complete (GPid     pid,
                  gint     status,
                  gpointer user_data);

static void
preview_clicked (GtkWidget *widget,
                 gpointer data);

static gboolean
accel_preview (GtkAccelGroup *group,
               GObject * obj,
               guint keyval,
               GdkModifierType mod,
               gpointer data);

static void
on_toc_changed (GtkWidget *widget,
                gpointer data);

static gboolean
accel_toc (GtkAccelGroup *group,
           GObject * obj,
           guint keyval,
           GdkModifierType mod,
           gpointer data);

static void
file_load_complete (MtxTextView *text_view,
                    const gchar *file,
                    gpointer data);

static void
on_new_text_buffer (MtxTextView *text_view,
                    gpointer data);

static void
on_curpos_changed (GtkTextBuffer *buffer,
                   GParamSpec *a2,
                   gpointer data);

static void
hovering_over_link (MtxTextView *text_view,
                    const gchar *link,
                    gpointer data);

static void
hovering_over_text (MtxTextView *text_view,
                    gpointer data);

static gboolean
mtx_viewer_is_current_tracked (MtxViewer *mvr);

static gboolean
mtx_viewer_is_current_skipping_toc (MtxViewer *mvr);

static void
mtx_viewer_widgets_set_sensitive (MtxViewer *mvr,
                                  gboolean enable);

static gchar *
_file_get_content_type (const gchar *path);

static gboolean
_is_text_and_markdown (const gchar *content_type,
                       gboolean *is_markdown);

static gint
_build_search_lists (MtxViewer *mvr,
                     GSList **markdown,
                     GSList **text,
                     gsize *base_length);

static void
_file_search (gpointer path,
              gpointer pod);

static gboolean
mtx_viewer_search_files (MtxViewer *mvr,
                         const gchar *text,
                         const gchar *file_complete,
                         GClosure* completer);

static gboolean
mtx_viewer_load_resource (MtxViewer *mvr,
                          const gchar *path,
                          const gchar *embed,
                          const gchar *file_complete,
                          GClosure *completer);

static void
do_open_welcome_page (MtxViewer *mvr);

static void
search_entry_activate (GtkEntry *entry, gpointer data);

static void
search_entry_icon_press (GtkEntry *entry,
                         gint position,
                         GdkEventButton *event,
                         gpointer data);

static gboolean
accel_search_entry_focus (GtkAccelGroup *group,
                          GObject *obj,
                          guint keyval,
                          GdkModifierType mod,
                          gpointer data);

static gboolean
accel_search_fore (GtkAccelGroup *group,
                   GObject *obj,
                   guint keyval,
                   GdkModifierType mod,
                   gpointer data);

static gboolean
accel_search_back (GtkAccelGroup *group,
                   GObject *obj,
                   guint keyval,
                   GdkModifierType mod,
                   gpointer data);

static void
nav_home_clicked (GtkWidget *button,
                  gpointer data);

static gboolean
accel_nav_home (GtkAccelGroup *group,
                GObject *obj,
                guint keyval,
                GdkModifierType mod,
                gpointer data);

static void
scroll_to_link_and_highlight (MtxViewer *mvr,
                              const MtxTextViewLinkInfo *link_info);

static gboolean
accel_link_fore (GtkAccelGroup *group,
                 GObject *obj,
                 guint keyval,
                 GdkModifierType mod,
                 gpointer data);

static gboolean
accel_link_back (GtkAccelGroup *group,
                 GObject *obj,
                 guint keyval,
                 GdkModifierType mod,
                 gpointer data);

static gboolean
accel_open_help (GtkAccelGroup *group,
                 GObject *obj,
                 guint keyval,
                 GdkModifierType mod,
                 gpointer data);

static gboolean
accel_edit_current (GtkAccelGroup *group,
                    GObject *obj,
                    guint keyval,
                    GdkModifierType mod,
                    gpointer data);

static gchar *
mtx_viewer_make_error_page (MtxViewer *mvr,
                            const gchar *message,
                            const gchar *back_link);

static void
_nav_unit_clear (MtxViewerNavUnit *unit,
                 MtxViewer *mvr);

static gboolean
viewer_destroy_me (GtkWidget *widget,
                   GdkEvent *event,
                   gpointer data);

static gboolean
viewer_key_pressed (GtkWidget *widget,
                    GdkEventKey *event,
                    gpointer data);

static GtkWidget *
mtx_viewer_button_icon_new_from_resource (const gchar *resource,
                                          const gint size_enum);

