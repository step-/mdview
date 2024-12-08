GtkWidget *
mtx_text_view_new();

MtxTextViewLinkTag *
mtx_text_view_get_link_tag_at_iter (MtxTextView *self,
                                    GtkTextIter *iter);

const MtxTextViewLinkInfo *
mtx_text_view_get_link_info_from_link_tag (MtxTextView *self,
                                           const MtxTextViewLinkTag *link_tag);

guint
mtx_text_view_link_count (MtxTextView *self);

const MtxTextViewLinkInfo *
mtx_text_view_link_info_get (MtxTextView *self,
                             const guint index_);

const MtxTextViewLinkInfo *
mtx_text_view_link_info_get_near_offset (MtxTextView *self,
                                         guint offset,
                                         const gint direction);

void
mtx_text_view_load_markup_cancel (MtxTextView *self);

gboolean
mtx_text_view_set_text (MtxTextView *self,
                        gchar **text,
                        const gchar *referrer,
                        const gboolean clear_text, /* free *text and text */
                        const gchar *file_complete,
                        GClosure *completer);

gboolean
mtx_text_view_load_file (MtxTextView *self,
                         const gchar *file,
                         const gchar *referrer,
                         const gboolean utf8_validate,
                         GClosure *completer);

void
mtx_text_view_set_image_directory (MtxTextView *self,
                                   const gchar *directory);

void
mtx_text_view_set_auto_lang_find (MtxTextView *self,
                                  const gboolean enable);

void
mtx_text_view_set_extensions (MtxTextView *self,
                              const MtxCmmExtensions flags);

gboolean
mtx_text_view_set_progress_fd (MtxTextView *self,
                               const gint fd);

const GRegex *
mtx_text_view_get_regex_astx (MtxTextView *self);

guint
mtx_text_view_get_toc_level (MtxTextView *self);

void
mtx_text_view_set_toc_level (MtxTextView *self,
                             const guint value);

void
mtx_text_view_set_tweaks (MtxTextView *self,
                          const MtxCmmTweaks flags);

const MtxCmmPageMeta *
mtx_text_view_fetch_page_meta (MtxTextView *self);

void
mtx_text_view_clear_line_highlights (MtxTextView *self,
                                     GtkTextMark *mark);

void
mtx_text_view_clear_page_highlights (MtxTextView *self);

gboolean
mtx_text_view_find_text (MtxTextView *self,
                         const gchar *search_text,
                         MtxTextViewSearchOptions options);

void
mtx_text_view_highlight_at_cursor (MtxTextView *self,
                                   const MtxTextViewHilightMode mode);

void
mtx_text_view_highlight_at_cursor_chars (MtxTextView *self,
                                         const guint chars,
                                         const MtxTextViewHilightMode mode);

void
mtx_text_view_cursor_to_top (MtxTextView *self);

gchar *
mtx_text_view_mmap_read_file (const gchar *path,
                              gsize *size,
                              const gboolean utf8_validate);

gchar *
mtx_text_view_auto_lang_find (MtxTextView *self,
                              const gchar *path);

gchar *
mtx_text_view_get_file_contents (MtxTextView *self,
                                 const gchar *path,
                                 gsize *size,
                                 const gboolean utf8_validate);

/***********************************************************/

static void mtx_text_view_class_init (MtxTextViewClass *klass);

static void
mtx_text_view_log_progress (MtxTextView *self,
                            const MtxTextViewProgress id);

static gboolean
mtx_text_view_buffer_insert_markup (GtkTextBuffer *buffer,
                                    GtkTextIter *iter,
                                    const gchar *markup,
                                    GError **error);

static gboolean
link_info_link_tag_equal (gconstpointer *a,
                          gconstpointer *b);

static void
set_cursor_and_signal_on_hover (MtxTextView *self,
                                gint x,
                                gint y);

static gboolean
motion_notify_event (MtxTextView *self,
                     GdkEventMotion *event);

static gboolean
visibility_notify_event (MtxTextView *self,
                         GdkEventVisibility *event);

static void
follow_if_link (MtxTextView *self,
                GtkTextIter *iter);

inline static void
_text_buffer_get_cursor (GtkTextBuffer *buffer,
                         GtkTextIter *iter);

static gboolean
key_press_event (MtxTextView *self,
                 GdkEventKey *event);

static gboolean
event_after (MtxTextView *self,
             GdkEvent *ev);

static gboolean
mtx_text_view_reset_buffer (MtxTextView *self);

static gboolean
mtx_text_view_swap_buffer (MtxTextView *self,
                           GtkTextBuffer *new,
                           GPtrArray *link_marks);

static GdkPixbuf *
get_pixbuf_from_id (MtxTextView *self,
                    const gint id,
                    const gchar *referrer);

static GPtrArray *
mtx_text_view_load_images_mark_links (MtxTextView *self,
                                      GtkTextBuffer *buffer,
                                      const gchar *referrer,
                                      GHashTable **dest_table);

static inline void
_get_string_pixel_size (MtxTextView *self,
                        gchar *string,
                        GtkTextTag *tag,
                        guint *width,
                        guint *height);

static void
_indent_slice_lines (MtxTextView *self,
                     const GtkTextIter *start,
                     const GtkTextIter *end,
                     const guint width,
                     GtkTextTag *base_margin,
                     GtkTextTag *base_indent);

static MtxTextViewPrivateRendered *
_get_rendered (MtxTextView *self,
               const GtkTextIter *start,
               GtkTextTag *tag);

static void
_free_rendered (MtxTextViewPrivateRendered *ptr);

static void
_text_buffer_set_invisible_to_eol (GtkTextBuffer *buffer,
                                   GtkTextIter *iter);

static void
mtx_text_view_buffer_indent_blockquote (MtxTextView *self,
                                        GtkTextBuffer *buffer,
                                        GtkTextTag *base_margin,
                                        GtkTextTag *base_indent);

static void
mtx_text_view_buffer_indent_li (MtxTextView *self,
                                GtkTextBuffer *buffer,
                                GtkTextTag *base_margin,
                                GtkTextTag *base_indent);

static void
mtx_text_view_buffer_delete_unichar_all (MtxTextView *self,
                                         GtkTextBuffer *buffer,
                                         gunichar code_point);

static void
mtx_text_view_buffer_indent_text (MtxTextView *self,
                                  GtkTextBuffer *buffer);

static void
mtx_text_view_load_markup_async (MtxTextView *object,
                                 gchar **markdown,
                                 const gboolean clear_markdown,
                                 GCancellable *cancellable,
                                 GAsyncReadyCallback callback,
                                 gpointer user_data);

static void
mtx_text_view_load_markup_thread_cb (GTask *task,
                                     gpointer source_object,
                                     gpointer task_data,
                                     GCancellable *cancellable);

static gboolean
mtx_text_view_load_markup_finish (GAsyncResult * result,
                                  GError **error);

static void
mtx_text_view_load_markup_loaded (GObject *object,
                                  GAsyncResult *result,
                                  gpointer user_data);

static void
mtx_text_view_load_markup_data_free (mtx_text_view_load_markup_data *data);

static void
mtx_text_view_init (MtxTextView *self);

static void
mtx_text_view_dispose (GObject *gobject);

static void
mtx_text_view_finalize (GObject *gobject);

