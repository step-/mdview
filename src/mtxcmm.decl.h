#ifdef MTX_DEBUG
static void mtx_dump_queue (gpointer instance,
                            int fd,
                            GQueue* queue,
                            gboolean print_junk);
#endif

void
mtx_cmm_parser_unit_new (MtxCmm *self,
                         const MtxCmmParserUnitType type,
                         const MtxCmmParserUnitFlag flag_mask);

MtxCmm *
mtx_cmm_new (MtxCmmOutput output);

gint
mtx_cmm_get_tag_val (MtxCmm *self,
                     const gchar *tag,
                     const MtxCmmTagInfo subject);

const gchar *
mtx_cmm_get_link_dest (MtxCmm *self,
                       const gint id);

gboolean
mtx_cmm_get_render_indent (MtxCmm *self);

gboolean
mtx_cmm_get_escape (MtxCmm *self);

gboolean
mtx_cmm_set_escape (MtxCmm *self,
                    gboolean escape);

MtxCmmExtensions
mtx_cmm_get_extensions (MtxCmm *self);

gboolean
mtx_cmm_set_extensions (MtxCmm *self,
                        const MtxCmmExtensions flags);

gboolean
mtx_cmm_set_progress_fd (MtxCmm *self,
                         const gint fd);

guint
mtx_cmm_get_toc_level (MtxCmm *self);

gboolean
mtx_cmm_set_toc_level (MtxCmm *self,
                       const guint value);

MtxCmmTweaks
mtx_cmm_get_tweaks (MtxCmm *self);

gboolean
mtx_cmm_set_tweaks (MtxCmm *self,
                    const MtxCmmTweaks flags);

const MtxCmmTags *
mtx_cmm_get_output_tags (MtxCmm *self);

MtxCmmOutput
mtx_cmm_get_output (MtxCmm *self);

gboolean
mtx_cmm_set_output (MtxCmm *self,
                    MtxCmmOutput output);

gboolean
mtx_cmm_got_blockquote (MtxCmm *self);

gboolean
mtx_cmm_got_img (MtxCmm *self);

gboolean
mtx_cmm_got_li (MtxCmm *self);

gboolean
mtx_cmm_got_link (MtxCmm *self);

GRegex *
mtx_cmm_regex_astx (MtxCmm *self);

gboolean
mtx_cmm_parser_is_unit_at_index (MtxCmm *self,
                                 const MtxCmmParserUnitType type,
                                 const MtxCmmParserUnitFlag flag,
                                 const int index);

gboolean
mtx_cmm_parser_top_unit_ends_line (MtxCmm *self);

gchar *
mtx_cmm_mtx (MtxCmm *self,
             gchar **markdown,
             gsize *size,
             MtxCmmPageMeta **meta,
             gchar **rtoc,
             const gboolean clear_markdown,
             GCancellable *cancellable);

/***********************************************************/

static void
mtx_cmm_finalize (GObject *object);

static void
mtx_cmm_class_init (MtxCmmClass *klass);

static void
mtx_cmm_init (MtxCmm *self);

static void
regex_table_el_destroy (GRegex **e);

static void
toc_entry_free (MtxCmmTocEntry *e);

static inline MtxCmmParserUnit *
mtx_cmm_parser_unit_cache_head (MtxCmm *self);

static inline MtxCmmParserUnit *
mtx_cmm_parser_unit_pop_head (MtxCmm *self);

static inline void
mtx_cmm_parser_unit_consume (MtxCmmParserUnit **unitptr);

static void
mtx_cmm_parser_unit_free (MtxCmm *self,
                          MtxCmmParserUnit *unit);

static inline void
mtx_cmm_parser_unit_clear (MtxCmmParserUnit *unit,
                           MtxCmm *self);

static void
mtx_cmm_parser_clear_queues (MtxCmm *self);

static MtxCmm *
mtx_cmm_new_internal (MtxCmmOutput output,
                      MtxCmm *caller);

static inline gchar*
mtx_cmm_make_code_ref (const guint id);

static inline gint
mtx_cmm_get_code_id (const gchar *ref);

static inline guint
mtx_cmm_stash_code (MtxCmm *self,
                    const gchar *code);

static inline const gchar *
mtx_cmm_get_code (MtxCmm *self,
                  const gint id);

static inline guint
mtx_cmm_stash_link_dest (MtxCmm *self,
                         const gchar *dest);

static void
mtx_cmm_mtx_reset (MtxCmm *self);

static inline gchar *
mtx_cmm_protect (MtxCmm *self,
                 const gchar *text);

static inline gchar *
mtx_strstr_code (const gchar *str);

static gint
mtx_cmm_string_release_protected (MtxCmm *self,
                                  GString *str,
                                  gchar *p);

inline static gboolean
mtx_strstrip_pango_markup (const gchar *string,
                        gchar **retptr);

static void
mtx_strstrip_pango_spans_fast (gchar *string);

static void
mtx_cmm_string_release_unmarked (MtxCmm *self,
                                 GString *str);

static gchar *
mtx_cmm_linkbuilder_pango (MtxCmm *self,
                           const gchar *text,
                           const gchar *dest,
                           const gchar *title,
                           const gint link_dest_id);

static gchar *
mtx_cmm_linkbuilder_html (MtxCmm *self,
                          const gchar *text,
                          const gchar *dest,
                          const gchar *title,
                          const gint link_dest_id);

static gchar *
mtx_cmm_linkbuilder_bare (MtxCmm *self,
                          const gchar *text,
                          const gchar *dest,
                          const gchar *title,
                          const gint link_dest_id);

static gchar *
mtx_cmm_linkbuilder_text (MtxCmm *self,
                          const gchar *text,
                          const gchar *dest,
                          const gchar *title,
                          const gint link_dest_id);

static gchar *
mtx_cmm_linkbuilder_ansi (MtxCmm *self,
                          const gchar *text,
                          const gchar *dest,
                          const gchar *title,
                          const gint link_dest_id);

static gchar *
mtx_cmm_imagebuilder_pango (MtxCmm *self,
                            const gchar *text,
                            const gchar *dest,
                            const gchar *title,
                            const gint link_dest_id);

static gchar *
mtx_cmm_imagebuilder_html (MtxCmm *self,
                           const gchar *text,
                           const gchar *dest,
                           const gchar *title,
                           const gint link_dest_id);

static gchar *
mtx_cmm_imagebuilder_text (MtxCmm *self,
                           const gchar *text,
                           const gchar *dest,
                           const gchar *title,
                           const gint link_dest_id);

static gchar *
mtx_cmm_imagebuilder_ansi (MtxCmm *self,
                           const gchar *text,
                           const gchar *dest,
                           const gchar *title,
                           const gint link_dest_id);

static MtxCmmPageMeta *
mtx_cmm_fetch_page_meta (MtxCmm *self);

static inline gchar *
mtx_cmm_format_link (MtxCmm *self,
                     const gchar *text,
                     const gchar *dest,
                     const gchar *title);

static inline gchar *
mtx_cmm_format_image (MtxCmm *self,
                      const gchar *text,
                      const gchar *dest,
                      const gchar *title);

static GRegex *
mtx_cmm_regex_directives (MtxCmm *self);

static gboolean
mtx_replace_directive_cb (const GMatchInfo *info,
                          GString *res,
                          gpointer data);

static void
mtx_cmm_string_replace_directives (MtxCmm *self,
                                   GString *str);

static void
mtx_cmm_log_progress (MtxCmm *self,
                      const MtxCmmProgress id);

static gboolean
mtx_insert_heading_link_cb (const GMatchInfo *info,
                            GString *res,
                            gpointer data);

static void
mtx_cmm_string_insert_heading_links (MtxCmm *self,
                                     MtxCmm *render,
                                     GString *str);

static gint
mtx_cmm_toc_hash_cmp (gconstpointer a,
                      gconstpointer b);

static gboolean
mtx_cmm_toc_entry_equal (gconstpointer *a,
                         gconstpointer *b);

static gboolean
mtx_delete_heading_link_cb (const GMatchInfo *info,
                            GString *res,
                            gpointer data);

static void
mtx_cmm_string_delete_heading_links (MtxCmm *self,
                                     GString *str);

static void
mtx_cmm_render_toc (MtxCmm *self);

static gchar *
mtx_cmm_make_toc_md (MtxCmm *self);

static GRegex *
mtx_cmm_regex_word_split (MtxCmm *self);

static gchar *
mtx_cmm_discover_auto_code_spans (MtxCmm *self,
                                  const gchar *text,
                                  const gchar *prefix,
                                  const gchar *suffix);

static void
mtx_cmm_replace_auto_code_spans (MtxCmm *self,
                                 GString *target,
                                 const guint start,
                                 const guint end,
                                 const gchar *prefix,
                                 const gchar *suffix);

static GRegex *
mtx_cmm_regex_dumb_quote_pairs (MtxCmm *self);

static gboolean
mtx_replace_dumb_quote_pair_cb (const GMatchInfo *info,
                                GString *res,
                                gpointer data);

static void
mtx_cmm_string_replace_smart_quotes (MtxCmm *self,
                                     GString *str);

static void
mtx_cmm_replace_smart_text (MtxCmm *self,
                            GString *target,
                            const guint start,
                            const guint end);

static GRegex *
mtx_cmm_regex_unipua (MtxCmm *self);

static gboolean
mtx_replace_unipua_cb (const GMatchInfo *info,
                       GString *res,
                       gpointer data);

static void
mtx_cmm_string_release_unipua (MtxCmm *self,
                               GString *str);

static const GRegex *
mtx_cmm_regex_tilde_code_fence (MtxCmm *self);

static guint
mtx_cmm_str_tilde_code_fence_max_len (MtxCmm *self, const gchar *str);

static GRegex *
mtx_cmm_regex_heading_link (MtxCmm *self);

static inline MtxCmmParserUnit *mtx_cmm_parser_get_unit_head (MtxCmm *);

static inline MtxCmmParserUnit *
mtx_cmm_parser_get_unit_head (MtxCmm *self);

static inline gboolean
mtx_cmm_parser_unit_ends_with_c (const MtxCmmParserUnit *u,
                                 const gchar c);

static const gchar
*mtx_cmm_parser_get_unit_arg_under (MtxCmm *,
                                   const MtxCmmParserUnitType,
                                   const MtxCmmParserUnitFlag,
                                   const guint);

static const gchar *
mtx_cmm_parser_get_unit_arg_under (MtxCmm *self,
                                   const MtxCmmParserUnitType type,
                                   const MtxCmmParserUnitFlag flag,
                                   const guint index);

static void
mtx_cmm_parser_merge_down_unit_arg (MtxCmm *self);

static void
mtx_cmm_parser_merge_down_unit_arg_inlines (MtxCmm *self);

static void
mtx_cmm_render_link_unit (MtxCmm *self,
                          MtxCmmParserUnit *unit,
                          MtxCmmAImgFormatter *formatter);

static inline int
mtx_cmm_render (MtxCmm *self,
                const MD_CHAR * input,
                MD_SIZE input_size,
                void (*process_output) (const MD_CHAR *, MD_SIZE, void *),
                void *userdata,
                unsigned parser_flags,
                unsigned renderer_flags);

inline static void
mtx_cmm_render_process_output (const MD_CHAR *out,
                               MD_SIZE length,
                               void *userdata);

static gint
_col_strlen (const gchar *p);

static void
mtx_cmm_extract_meta_data (MtxCmmPageMeta *meta,
                           GString *str);

