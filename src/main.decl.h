int
main (int argc, char **argv);

/***********************************************************/

static void
html_doc_start (const gint fd,
                MtxCmmTweaks tweaks,
                const gint html_css,
                const gchar *html_base);

static void
html_doc_end (const gint fd,
              MtxCmmTweaks tweaks,
              const gint html_css,
              const gboolean add_js);

static void
usage (MtxCmmOutput output);

static gboolean
is_valid_scheme (const char *scheme,
                 const gboolean console_output);

