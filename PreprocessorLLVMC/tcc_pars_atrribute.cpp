///* Parse __attribute__((...)) GNUC extension. */
//static void parse_attribute(AttributeDef* ad)
//{
//    int t, n;
//    char* astr;
//
//redo:
//    if (tok != TOK_ATTRIBUTE1 && tok != TOK_ATTRIBUTE2)
//        return;
//    next();
//    skip('(');
//    skip('(');
//    while (tok != ')') {
//        if (tok < TOK_IDENT)
//            expect("attribute name");
//        t = tok;
//        next();
//        switch (t) {
//        case TOK_CLEANUP1:
//        case TOK_CLEANUP2:
//        {
//            Sym* s;
//
//            skip('(');
//            s = sym_find(tok);
//            if (!s) {
//                tcc_warning_c(warn_implicit_function_declaration)(
//                    "implicit declaration of function '%s'", get_tok_str(tok, &tokc));
//                s = external_global_sym(tok, &func_old_type);
//            }
//            else if ((s->type.t & VT_BTYPE) != VT_FUNC)
//                tcc_error("'%s' is not declared as function", get_tok_str(tok, &tokc));
//            ad->cleanup_func = s;
//            next();
//            skip(')');
//            break;
//        }
//        case TOK_CONSTRUCTOR1:
//        case TOK_CONSTRUCTOR2:
//            ad->f.func_ctor = 1;
//            break;
//        case TOK_DESTRUCTOR1:
//        case TOK_DESTRUCTOR2:
//            ad->f.func_dtor = 1;
//            break;
//        case TOK_ALWAYS_INLINE1:
//        case TOK_ALWAYS_INLINE2:
//            ad->f.func_alwinl = 1;
//            break;
//        case TOK_SECTION1:
//        case TOK_SECTION2:
//            skip('(');
//            astr = parse_mult_str("section name")->data;
//            ad->section = find_section(tcc_state, astr);
//            skip(')');
//            break;
//        case TOK_ALIAS1:
//        case TOK_ALIAS2:
//            skip('(');
//            astr = parse_mult_str("alias(\"target\")")->data;
//            /* save string as token, for later */
//            ad->alias_target = tok_alloc_const(astr);
//            skip(')');
//            break;
//        case TOK_VISIBILITY1:
//        case TOK_VISIBILITY2:
//            skip('(');
//            astr = parse_mult_str("visibility(\"default|hidden|internal|protected\")")->data;
//            if (!strcmp(astr, "default"))
//                ad->a.visibility = STV_DEFAULT;
//            else if (!strcmp(astr, "hidden"))
//                ad->a.visibility = STV_HIDDEN;
//            else if (!strcmp(astr, "internal"))
//                ad->a.visibility = STV_INTERNAL;
//            else if (!strcmp(astr, "protected"))
//                ad->a.visibility = STV_PROTECTED;
//            else
//                expect("visibility(\"default|hidden|internal|protected\")");
//            skip(')');
//            break;
//        case TOK_ALIGNED1:
//        case TOK_ALIGNED2:
//            if (tok == '(') {
//                next();
//                n = expr_const();
//                if (n <= 0 || (n & (n - 1)) != 0)
//                    tcc_error("alignment must be a positive power of two");
//                skip(')');
//            }
//            else {
//                n = MAX_ALIGN;
//            }
//            ad->a.aligned = exact_log2p1(n);
//            if (n != 1 << (ad->a.aligned - 1))
//                tcc_error("alignment of %d is larger than implemented", n);
//            break;
//        case TOK_PACKED1:
//        case TOK_PACKED2:
//            ad->a.packed = 1;
//            break;
//        case TOK_WEAK1:
//        case TOK_WEAK2:
//            ad->a.weak = 1;
//            break;
//        case TOK_NODEBUG1:
//        case TOK_NODEBUG2:
//            ad->a.nodebug = 1;
//            break;
//        case TOK_UNUSED1:
//        case TOK_UNUSED2:
//            /* currently, no need to handle it because tcc does not
//               track unused objects */
//            break;
//        case TOK_NORETURN1:
//        case TOK_NORETURN2:
//            ad->f.func_noreturn = 1;
//            break;
//        case TOK_CDECL1:
//        case TOK_CDECL2:
//        case TOK_CDECL3:
//            ad->f.func_call = FUNC_CDECL;
//            break;
//        case TOK_STDCALL1:
//        case TOK_STDCALL2:
//        case TOK_STDCALL3:
//            ad->f.func_call = FUNC_STDCALL;
//            break;
//#ifdef TCC_TARGET_I386
//        case TOK_REGPARM1:
//        case TOK_REGPARM2:
//            skip('(');
//            n = expr_const();
//            if (n > 3)
//                n = 3;
//            else if (n < 0)
//                n = 0;
//            if (n > 0)
//                ad->f.func_call = FUNC_FASTCALL1 + n - 1;
//            skip(')');
//            break;
//        case TOK_FASTCALL1:
//        case TOK_FASTCALL2:
//        case TOK_FASTCALL3:
//            ad->f.func_call = FUNC_FASTCALLW;
//            break;
//#endif
//        case TOK_MODE:
//            skip('(');
//            switch (tok) {
//            case TOK_MODE_DI:
//                ad->attr_mode = VT_LLONG + 1;
//                break;
//            case TOK_MODE_QI:
//                ad->attr_mode = VT_BYTE + 1;
//                break;
//            case TOK_MODE_HI:
//                ad->attr_mode = VT_SHORT + 1;
//                break;
//            case TOK_MODE_SI:
//            case TOK_MODE_word:
//                ad->attr_mode = VT_INT + 1;
//                break;
//            default:
//                tcc_warning("__mode__(%s) not supported\n", get_tok_str(tok, NULL));
//                break;
//            }
//            next();
//            skip(')');
//            break;
//        case TOK_DLLEXPORT:
//            ad->a.dllexport = 1;
//            break;
//        case TOK_NODECORATE:
//            ad->a.nodecorate = 1;
//            break;
//        case TOK_DLLIMPORT:
//            ad->a.dllimport = 1;
//            break;
//        default:
//            tcc_warning_c(warn_unsupported)("'%s' attribute ignored", get_tok_str(t, NULL));
//            /* skip parameters */
//            if (tok == '(') {
//                int parenthesis = 0;
//                do {
//                    if (tok == '(')
//                        parenthesis++;
//                    else if (tok == ')')
//                        parenthesis--;
//                    next();
//                } while (parenthesis && tok != -1);
//            }
//            break;
//        }
//        if (tok != ',')
//            break;
//        next();
//    }
//    skip(')');
//    skip(')');
//    goto redo;
//}