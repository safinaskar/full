// TODO: если запустить в tty и набрать Ctrl-J, то выведет <10>, а потом сам Ctrl-J, и курсор сместится вниз, но не влево. Может, это баг?

#define _POSIX_C_SOURCE 1
#define _XOPEN_SOURCE 500 // For Interix

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <locale.h>
#include <wchar.h>

#include <unistd.h>

#include <err.h>

#ifndef NO_INTERACTIVE // For strange environments
# include <termios.h>
#endif /* NO_INTERACTIVE */

#include <libsh.h>
#include <libsh/cxx.hpp>

using namespace std::literals;

const wchar_t *name_of[128] = {L"NUL", L"SOH", L"STX", L"ETX", L"EOT",
  L"ENQ", L"ACK", L"BEL", L"BS", L"HT", L"LF", L"VT", L"FF", L"CR", L"SO",
  L"SI", L"DLE", L"DC1", L"DC2", L"DC3", L"DC4", L"NAK", L"SYN", L"ETB",
  L"CAN", L"EM", L"SUB", L"ESC", L"FS", L"GS", L"RS", L"US"};

const wchar_t *c_escape_array[128] = {NULL}; // This must be!

void dump_char(int c)
{
  wprintf(L"\\%03o %3d \\x%02x ^%c  %-3ls %ls\n", c, c, c, (c + 64) % 128, name_of[c],
    c_escape_array[c] == NULL ? L"  " : c_escape_array[c]);
}

void dump(void)
{
  fputws(L"Oct  Dec Hex  Car Nam C\n", stdout);
  for (int i = 0; i != 32; ++i)
    {
      dump_char(i);
    }
  dump_char(127);
  exit (EXIT_SUCCESS);
}

int main(int, char *argv[])
{
  name_of[127] = L"DEL";

  c_escape_array['\a'] = L"\\a";
  c_escape_array['\b'] = L"\\b";
  c_escape_array['\t'] = L"\\t";
  c_escape_array['\n'] = L"\\n";
  c_escape_array['\v'] = L"\\v";
  c_escape_array['\f'] = L"\\f";
  c_escape_array['\r'] = L"\\r";

  enum {oct = 0, dec, hex};
  enum {control, high, all};
  enum {cf_normal, caret, name};

  int format = dec;
  int set = control;
  int control_format = cf_normal;

  int c_escape = 0;
  int tab = 1;

  int wide = 1;

  int color = isatty(1);

  sh_init (argv[0]);
  sh_arg_parse (&argv, "Usage: "s + sh_get_program () + " [OPTION]...\n"s, "",
    sh_arg_make_opt ({'o'}, {"oct"  }, sh_arg_optional, [&](const char *){ format         = oct;   }, NULL, "octal"),
    sh_arg_make_opt ({'x'}, {"hex"  }, sh_arg_optional, [&](const char *){ format         = hex;   }, NULL, "hexademical\n"),

    sh_arg_make_opt ({'h'}, {"high" }, sh_arg_optional, [&](const char *){ set            = high;  }, NULL, "control characters + characters with high codes"),
    sh_arg_make_opt ({'a'}, {"all"  }, sh_arg_optional, [&](const char *){ set            = all;   }, NULL, "all characters\n"),

    sh_arg_make_opt ({'C'}, {"caret"}, sh_arg_optional, [&](const char *){ control_format = caret; }, NULL, "use caret notation"),
    sh_arg_make_opt ({'n'}, {"name" }, sh_arg_optional, [&](const char *){ control_format = name;  }, NULL, "print control characters by name\n"),

    sh_arg_make_opt ({'c'}, {       }, sh_arg_optional, [&](const char *){ c_escape       = 1;     }, NULL, "C escape notation"),
    sh_arg_make_opt ({'t'}, {"tab"  }, sh_arg_optional, [&](const char *){ tab            = 0;     }, NULL, "tabs and new lines are not special\n"),

    sh_arg_make_opt ({'1'}, {       }, sh_arg_optional, [&](const char *){ wide           = 0;     }, NULL, "1-byte input\n"),

    sh_arg_make_opt ({   }, {"color"}, sh_arg_optional, [&](const char *){ color          = 1;     }, NULL, "color\n"),

    sh_arg_make_opt ({'d'}, {"dump" }, sh_arg_optional, [&](const char *){ dump ();                }, NULL, "dump")
  );
  sh_arg_end (argv);

  if (!wide && set == control)set = high;

#ifndef NO_INTERACTIVE
  {
    struct termios term;
    tcgetattr(fileno(stdin), &term);

    // Copied from man cfmakeraw (I cannot write cfmakeraw, because Interix has no this function)
    term.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR |
      ICRNL | IXON);
    term.c_oflag &= ~OPOST;
    term.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    term.c_cflag &= ~(CSIZE | PARENB);
    term.c_cflag |= CS8;
    // End of man

    term.c_lflag |= ISIG;
    tcsetattr(fileno(stdin), 0, &term);
  }
#endif /* NO_INTERACTIVE */

  if (wide)setlocale(LC_ALL, "");

  int c; // We assume int is not less than wint_t

  for (;;){
    if (wide){
      errno = 0;
      c = getwchar(); // Хорошо бы сделать проверку errno. Вдруг мы читаем из сокета, а там какая-нибудь ошибка
      if (c == EOF){
        if (errno == EILSEQ){
          err (EXIT_FAILURE, NULL);
        }
        break;
      }
    }else{
      c = getchar();
      if (c == (int)WEOF){
        break;
      }
    }
    if ((c < 32 || c >= 127) && color && set != all)
      {
        fputws(L"\033[1;31m", stdout);
      }
    if (tab && c == '\t'){
      fputws(L"<------>", stdout);
    }else if (c_escape && c_escape_array[c] != NULL){
      fputws(c_escape_array[c], stdout);
    }else if ((c < 32 || c == 127) &&
      control_format != cf_normal){
      switch (control_format){
        case cf_normal: // To avoid compiler warnings
        case caret:
          wprintf(L"^%c", (c + 64) % 128);
          break;
        case name:
          wprintf(L"<%ls>", name_of[c]);
          break;
      }
    }else if (c < 32 || c == 127 ||
      (set == high && c >= 128) || set == all){
      switch (format){
        case oct: wprintf(L"\\%03o", c); break;
        case dec: wprintf(L"<%d>", c); break;
        case hex: wprintf(L"\\x%02x", c); break;
      }
    }else
      {
        putwchar(c);
      }
    if ((c < 32 || c >= 127) && color && set != all)
      {
        fputws(L"\033[0m", stdout);
      }
    if (tab && c == '\n')putwchar(L'\n');
    fflush(stdout);
  }
}
