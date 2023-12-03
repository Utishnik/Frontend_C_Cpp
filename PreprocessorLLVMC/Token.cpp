//tcc code
#include "tcc.h"
static inline void next_nomacro1(void)
{
    int t, c, is_long, len;
    TokenSym* ts;
    uint8_t* p, * p1;
    unsigned int h;

    p = file->buf_ptr;
redo_no_start:
    c = *p;
    switch (c) {
    case ' ':
    case '\t':
        tok = c;
        p++;
    maybe_space:
        if (parse_flags & PARSE_FLAG_SPACES)
            goto keep_tok_flags;
        while (isidnum_table[*p - CH_EOF] & IS_SPC)
            ++p;
        goto redo_no_start;
    case '\f':
    case '\v':
    case '\r':
        p++;
        goto redo_no_start;
    case '\\':
        /* first look if it is in fact an end of buffer */
        c = handle_stray(&p);
        if (c == '\\')
            goto parse_simple;
        if (c == CH_EOF) {
            TCCState* s1 = tcc_state;
            if ((parse_flags & PARSE_FLAG_LINEFEED)
                && !(tok_flags & TOK_FLAG_EOF)) {
                tok_flags |= TOK_FLAG_EOF;
                tok = TOK_LINEFEED;
                goto keep_tok_flags;
            }
            else if (!(parse_flags & PARSE_FLAG_PREPROCESS)) {
                tok = TOK_EOF;
            }
            else if (s1->ifdef_stack_ptr != file->ifdef_stack_ptr) {
                tcc_error("missing #endif");
            }
            else if (s1->include_stack_ptr == s1->include_stack) {
                /* no include left : end of file. */
                tok = TOK_EOF;
            }
            else {
                tok_flags &= ~TOK_FLAG_EOF;
                /* pop include file */

                /* test if previous '#endif' was after a #ifdef at
                   start of file */
                if (tok_flags & TOK_FLAG_ENDIF) {
#ifdef INC_DEBUG
                    printf("#endif %s\n", get_tok_str(file->ifndef_macro_saved, NULL));
#endif
                    search_cached_include(s1, file->filename, 1)
                        ->ifndef_macro = file->ifndef_macro_saved;
                    tok_flags &= ~TOK_FLAG_ENDIF;
                }

                /* add end of include file debug info */
                tcc_debug_eincl(tcc_state);
                /* pop include stack */
                tcc_close();
                s1->include_stack_ptr--;
                p = file->buf_ptr;
                if (p == file->buffer)
                    tok_flags = TOK_FLAG_BOF;
                tok_flags |= TOK_FLAG_BOL;
                goto redo_no_start;
            }
        }
        else {
            goto redo_no_start;
        }
        break;

    case '\n':
        file->line_num++;
        tok_flags |= TOK_FLAG_BOL;
        p++;
    maybe_newline:
        if (0 == (parse_flags & PARSE_FLAG_LINEFEED))
            goto redo_no_start;
        tok = TOK_LINEFEED;
        goto keep_tok_flags;

    case '#':
        /* XXX: simplify */
        PEEKC(c, p);
        if ((tok_flags & TOK_FLAG_BOL) &&
            (parse_flags & PARSE_FLAG_PREPROCESS)) {
            file->buf_ptr = p;
            preprocess(tok_flags & TOK_FLAG_BOF);
            p = file->buf_ptr;
            goto maybe_newline;
        }
        else {
            if (c == '#') {
                p++;
                tok = TOK_TWOSHARPS;
            }
            else {
#if !defined(TCC_TARGET_ARM)
                if (parse_flags & PARSE_FLAG_ASM_FILE) {
                    p = parse_line_comment(p - 1);
                    goto redo_no_start;
                }
                else
#endif
                {
                    tok = '#';
                }
            }
        }
        break;

        /* dollar is allowed to start identifiers when not parsing asm */
    case '$':
        if (!(isidnum_table[c - CH_EOF] & IS_ID)
            || (parse_flags & PARSE_FLAG_ASM_FILE))
            goto parse_simple;

    case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h':
    case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't':
    case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z':
    case 'A': case 'B': case 'C': case 'D':
    case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K':
    case 'M': case 'N': case 'O': case 'P':
    case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z':
    case '_':
    parse_ident_fast:
        p1 = p;
        h = TOK_HASH_INIT;
        h = TOK_HASH_FUNC(h, c);
        while (c = *++p, isidnum_table[c - CH_EOF] & (IS_ID | IS_NUM))
            h = TOK_HASH_FUNC(h, c);
        len = p - p1;
        if (c != '\\') {
            TokenSym** pts;

            /* fast case : no stray found, so we have the full token
               and we have already hashed it */
            h &= (TOK_HASH_SIZE - 1);
            pts = &hash_ident[h];
            for (;;) {
                ts = *pts;
                if (!ts)
                    break;
                if (ts->len == len && !memcmp(ts->str, p1, len))
                    goto token_found;
                pts = &(ts->hash_next);
            }
            ts = tok_alloc_new(pts, (char*)p1, len);
        token_found:;
        }
        else {
            /* slower case */
            cstr_reset(&tokcstr);
            cstr_cat(&tokcstr, (char*)p1, len);
            p--;
            PEEKC(c, p);
        parse_ident_slow:
            while (isidnum_table[c - CH_EOF] & (IS_ID | IS_NUM))
            {
                cstr_ccat(&tokcstr, c);
                PEEKC(c, p);
            }
            ts = tok_alloc(tokcstr.data, tokcstr.size);
        }
        tok = ts->tok;
        break;
    case 'L':
        t = p[1];
        if (t != '\\' && t != '\'' && t != '\"') {
            /* fast case */
            goto parse_ident_fast;
        }
        else {
            PEEKC(c, p);
            if (c == '\'' || c == '\"') {
                is_long = 1;
                goto str_const;
            }
            else {
                cstr_reset(&tokcstr);
                cstr_ccat(&tokcstr, 'L');
                goto parse_ident_slow;
            }
        }
        break;

    case '0': case '1': case '2': case '3':
    case '4': case '5': case '6': case '7':
    case '8': case '9':
        t = c;
        PEEKC(c, p);
        /* after the first digit, accept digits, alpha, '.' or sign if
           prefixed by 'eEpP' */
    parse_num:
        cstr_reset(&tokcstr);
        for (;;) {
            cstr_ccat(&tokcstr, t);
            if (!((isidnum_table[c - CH_EOF] & (IS_ID | IS_NUM))
                || c == '.'
                || ((c == '+' || c == '-')
                    && (((t == 'e' || t == 'E')
                        && !(parse_flags & PARSE_FLAG_ASM_FILE
                            /* 0xe+1 is 3 tokens in asm */
                            && ((char*)tokcstr.data)[0] == '0'
                            && toup(((char*)tokcstr.data)[1]) == 'X'))
                        || t == 'p' || t == 'P'))))
                break;
            t = c;
            PEEKC(c, p);
        }
        /* We add a trailing '\0' to ease parsing */
        cstr_ccat(&tokcstr, '\0');
        tokc.str.size = tokcstr.size;
        tokc.str.data = tokcstr.data;
        tok = TOK_PPNUM;
        break;

    case '.':
        /* special dot handling because it can also start a number */
        PEEKC(c, p);
        if (isnum(c)) {
            t = '.';
            goto parse_num;
        }
        else if ((isidnum_table['.' - CH_EOF] & IS_ID)
            && (isidnum_table[c - CH_EOF] & (IS_ID | IS_NUM))) {
            *--p = c = '.';
            goto parse_ident_fast;
        }
        else if (c == '.') {
            PEEKC(c, p);
            if (c == '.') {
                p++;
                tok = TOK_DOTS;
            }
            else {
                *--p = '.'; /* may underflow into file->unget[] */
                tok = '.';
            }
        }
        else {
            tok = '.';
        }
        break;
    case '\'':
    case '\"':
        is_long = 0;
    str_const:
        cstr_reset(&tokcstr);
        if (is_long)
            cstr_ccat(&tokcstr, 'L');
        cstr_ccat(&tokcstr, c);
        p = parse_pp_string(p, c, &tokcstr);
        cstr_ccat(&tokcstr, c);
        cstr_ccat(&tokcstr, '\0');
        tokc.str.size = tokcstr.size;
        tokc.str.data = tokcstr.data;
        tok = TOK_PPSTR;
        break;

    case '<':
        PEEKC(c, p);
        if (c == '=') {
            p++;
            tok = TOK_LE;
        }
        else if (c == '<') {
            PEEKC(c, p);
            if (c == '=') {
                p++;
                tok = TOK_A_SHL;
            }
            else {
                tok = TOK_SHL;
            }
        }
        else {
            tok = TOK_LT;
        }
        break;
    case '>':
        PEEKC(c, p);
        if (c == '=') {
            p++;
            tok = TOK_GE;
        }
        else if (c == '>') {
            PEEKC(c, p);
            if (c == '=') {
                p++;
                tok = TOK_A_SAR;
            }
            else {
                tok = TOK_SAR;
            }
        }
        else {
            tok = TOK_GT;
        }
        break;

    case '&':
        PEEKC(c, p);
        if (c == '&') {
            p++;
            tok = TOK_LAND;
        }
        else if (c == '=') {
            p++;
            tok = TOK_A_AND;
        }
        else {
            tok = '&';
        }
        break;

    case '|':
        PEEKC(c, p);
        if (c == '|') {
            p++;
            tok = TOK_LOR;
        }
        else if (c == '=') {
            p++;
            tok = TOK_A_OR;
        }
        else {
            tok = '|';
        }
        break;

    case '+':
        PEEKC(c, p);
        if (c == '+') {
            p++;
            tok = TOK_INC;
        }
        else if (c == '=') {
            p++;
            tok = TOK_A_ADD;
        }
        else {
            tok = '+';
        }
        break;

    case '-':
        PEEKC(c, p);
        if (c == '-') {
            p++;
            tok = TOK_DEC;
        }
        else if (c == '=') {
            p++;
            tok = TOK_A_SUB;
        }
        else if (c == '>') {
            p++;
            tok = TOK_ARROW;
        }
        else {
            tok = '-';
        }
        break;

        PARSE2('!', '!', '=', TOK_NE)
            PARSE2('=', '=', '=', TOK_EQ)
            PARSE2('*', '*', '=', TOK_A_MUL)
            PARSE2('%', '%', '=', TOK_A_MOD)
            PARSE2('^', '^', '=', TOK_A_XOR)

            /* comments or operator */
    case '/':
        PEEKC(c, p);
        if (c == '*') {
            p = parse_comment(p);
            /* comments replaced by a blank */
            tok = ' ';
            goto maybe_space;
        }
        else if (c == '/') {
            p = parse_line_comment(p);
            tok = ' ';
            goto maybe_space;
        }
        else if (c == '=') {
            p++;
            tok = TOK_A_DIV;
        }
        else {
            tok = '/';
        }
        break;

        /* simple tokens */
    case '(':
    case ')':
    case '[':
    case ']':
    case '{':
    case '}':
    case ',':
    case ';':
    case ':':
    case '?':
    case '~':
    case '@': /* only used in assembler */
    parse_simple:
        tok = c;
        p++;
        break;
    default:
        if (c >= 0x80 && c <= 0xFF) /* utf8 identifiers */
            goto parse_ident_fast;
        if (parse_flags & PARSE_FLAG_ASM_FILE)
            goto parse_simple;
        tcc_error("unrecognized character \\x%02x", c);
        break;
    }
    tok_flags = 0;
keep_tok_flags:
    file->buf_ptr = p;
#if defined(PARSE_DEBUG)
    printf("token = %d %s\n", tok, get_tok_str(tok, &tokc));
#endif
}
