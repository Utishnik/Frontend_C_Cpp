#include <vector>

void Preprocessor::SuggestTypoedDirective(const Token& Tok,
    StringRef Directive) const {
    // If this is a `.S` file, treat unknown # directives as non-preprocessor
    // directives.
    if (getLangOpts().AsmPreprocessor) return;

    std::vector<StringRef> Candidates = {
        "if", "ifdef", "ifndef", "elif", "else", "endif"
    };
    if (LangOpts.C23 || LangOpts.CPlusPlus23)
        Candidates.insert(Candidates.end(), { "elifdef", "elifndef" });

    if (std::optional<StringRef> Sugg = findSimilarStr(Directive, Candidates)) {
        // Directive cannot be coming from macro.
        assert(Tok.getLocation().isFileID());
        CharSourceRange DirectiveRange = CharSourceRange::getCharRange(
            Tok.getLocation(),
            Tok.getLocation().getLocWithOffset(Directive.size()));
        StringRef SuggValue = *Sugg;

        auto Hint = FixItHint::CreateReplacement(DirectiveRange, SuggValue);
        Diag(Tok, diag::warn_pp_invalid_directive) << 1 << SuggValue << Hint;
    }
}

//void Preprocessor_HandleDirective(Token& Result) {
//    // FIXME: Traditional: # with whitespace before it not recognized by K&R?
//
//    // We just parsed a # character at the start of a line, so we're in directive
//    // mode.  Tell the lexer this so any newlines we see will be converted into an
//    // EOD token (which terminates the directive).
//    CurPPLexer->ParsingPreprocessorDirective = true;
//    if (CurLexer) CurLexer->SetKeepWhitespaceMode(false);
//
//    bool ImmediatelyAfterTopLevelIfndef =
//        CurPPLexer->MIOpt.getImmediatelyAfterTopLevelIfndef();
//    CurPPLexer->MIOpt.resetImmediatelyAfterTopLevelIfndef();
//
//    ++NumDirectives;
//
//    // We are about to read a token.  For the multiple-include optimization FA to
//    // work, we have to remember if we had read any tokens *before* this
//    // pp-directive.
//    bool ReadAnyTokensBeforeDirective = CurPPLexer->MIOpt.getHasReadAnyTokensVal();
//
//    // Save the '#' token in case we need to return it later.
//    Token SavedHash = Result;
//
//    // Read the next token, the directive flavor.  This isn't expanded due to
//    // C99 6.10.3p8.
//    LexUnexpandedToken(Result);
//
//    // C99 6.10.3p11: Is this preprocessor directive in macro invocation?  e.g.:
//    //   #define A(x) #x
//    //   A(abc
//    //     #warning blah
//    //   def)
//    // If so, the user is relying on undefined behavior, emit a diagnostic. Do
//    // not support this for #include-like directives, since that can result in
//    // terrible diagnostics, and does not work in GCC.
//    if (InMacroArgs) {
//        if (IdentifierInfo* II = Result.getIdentifierInfo()) {
//            switch (II->getPPKeywordID()) {
//            case tok::pp_include:
//            case tok::pp_import:
//            case tok::pp_include_next:
//            case tok::pp___include_macros:
//            case tok::pp_pragma:
//                Diag(Result, diag::err_embedded_directive) << II->getName();
//                Diag(*ArgMacro, diag::note_macro_expansion_here)
//                    << ArgMacro->getIdentifierInfo();
//                DiscardUntilEndOfDirective();
//                return;
//            default:
//                break;
//            }
//        }
//        Diag(Result, diag::ext_embedded_directive);
//    }
//
//    // Temporarily enable macro expansion if set so
//    // and reset to previous state when returning from this function.
//    ResetMacroExpansionHelper helper(this);
//
//    if (SkippingUntilPCHThroughHeader || SkippingUntilPragmaHdrStop)
//        return HandleSkippedDirectiveWhileUsingPCH(Result, SavedHash.getLocation());
//
//    switch (Result.getKind()) {
//    case tok::eod:
//        // Ignore the null directive with regards to the multiple-include
//        // optimization, i.e. allow the null directive to appear outside of the
//        // include guard and still enable the multiple-include optimization.
//        CurPPLexer->MIOpt.SetReadToken(ReadAnyTokensBeforeDirective);
//        return;   // null directive.
//    case tok::code_completion:
//        setCodeCompletionReached();
//        if (CodeComplete)
//            CodeComplete->CodeCompleteDirective(
//                CurPPLexer->getConditionalStackDepth() > 0);
//        return;
//    case tok::numeric_constant:  // # 7  GNU line marker directive.
//        // In a .S file "# 4" may be a comment so don't treat it as a preprocessor
//        // directive. However do permit it in the predefines file, as we use line
//        // markers to mark the builtin macros as being in a system header.
//        if (getLangOpts().AsmPreprocessor &&
//            SourceMgr.getFileID(SavedHash.getLocation()) != getPredefinesFileID())
//            break;
//        return HandleDigitDirective(Result);
//    default:
//        IdentifierInfo* II = Result.getIdentifierInfo();
//        if (!II) break; // Not an identifier.
//
//        // Ask what the preprocessor keyword ID is.
//        switch (II->getPPKeywordID()) {
//        default: break;
//            // C99 6.10.1 - Conditional Inclusion.
//        case tok::pp_if:
//            return HandleIfDirective(Result, SavedHash, ReadAnyTokensBeforeDirective);
//        case tok::pp_ifdef:
//            return HandleIfdefDirective(Result, SavedHash, false,
//                true /*not valid for miopt*/);
//        case tok::pp_ifndef:
//            return HandleIfdefDirective(Result, SavedHash, true,
//                ReadAnyTokensBeforeDirective);
//        case tok::pp_elif:
//        case tok::pp_elifdef:
//        case tok::pp_elifndef:
//            return HandleElifFamilyDirective(Result, SavedHash, II->getPPKeywordID());
//
//        case tok::pp_else:
//            return HandleElseDirective(Result, SavedHash);
//        case tok::pp_endif:
//            return HandleEndifDirective(Result);
//
//            // C99 6.10.2 - Source File Inclusion.
//        case tok::pp_include:
//            // Handle #include.
//            return HandleIncludeDirective(SavedHash.getLocation(), Result);
//        case tok::pp___include_macros:
//            // Handle -imacros.
//            return HandleIncludeMacrosDirective(SavedHash.getLocation(), Result);
//
//            // C99 6.10.3 - Macro Replacement.
//        case tok::pp_define:
//            return HandleDefineDirective(Result, ImmediatelyAfterTopLevelIfndef);
//        case tok::pp_undef:
//            return HandleUndefDirective();
//
//            // C99 6.10.4 - Line Control.
//        case tok::pp_line:
//            return HandleLineDirective();
//
//            // C99 6.10.5 - Error Directive.
//        case tok::pp_error:
//            return HandleUserDiagnosticDirective(Result, false);
//
//            // C99 6.10.6 - Pragma Directive.
//        case tok::pp_pragma:
//            return HandlePragmaDirective({ PIK_HashPragma, SavedHash.getLocation() });
//
//            // GNU Extensions.
//        case tok::pp_import:
//            return HandleImportDirective(SavedHash.getLocation(), Result);
//        case tok::pp_include_next:
//            return HandleIncludeNextDirective(SavedHash.getLocation(), Result);
//
//        case tok::pp_warning:
//            if (LangOpts.CPlusPlus)
//                Diag(Result, LangOpts.CPlusPlus23
//                    ? diag::warn_cxx23_compat_warning_directive
//                    : diag::ext_pp_warning_directive)
//                << /*C++23*/ 1;
//            else
//                Diag(Result, LangOpts.C23 ? diag::warn_c23_compat_warning_directive
//                    : diag::ext_pp_warning_directive)
//                << /*C23*/ 0;
//
//            return HandleUserDiagnosticDirective(Result, true);
//        case tok::pp_ident:
//            return HandleIdentSCCSDirective(Result);
//        case tok::pp_sccs:
//            return HandleIdentSCCSDirective(Result);
//        case tok::pp_assert:
//            //isExtension = true;  // FIXME: implement #assert
//            break;
//        case tok::pp_unassert:
//            //isExtension = true;  // FIXME: implement #unassert
//            break;
//
//        case tok::pp___public_macro:
//            if (getLangOpts().Modules || getLangOpts().ModulesLocalVisibility)
//                return HandleMacroPublicDirective(Result);
//            break;
//
//        case tok::pp___private_macro:
//            if (getLangOpts().Modules || getLangOpts().ModulesLocalVisibility)
//                return HandleMacroPrivateDirective();
//            break;
//        }
//        break;
//    }
//
//    // If this is a .S file, treat unknown # directives as non-preprocessor
//    // directives.  This is important because # may be a comment or introduce
//    // various pseudo-ops.  Just return the # token and push back the following
//    // token to be lexed next time.
//    if (getLangOpts().AsmPreprocessor) {
//        auto Toks = std::make_unique<Token[]>(2);
//        // Return the # and the token after it.
//        Toks[0] = SavedHash;
//        Toks[1] = Result;
//
//        // If the second token is a hashhash token, then we need to translate it to
//        // unknown so the token lexer doesn't try to perform token pasting.
//        if (Result.is(tok::hashhash))
//            Toks[1].setKind(tok::unknown);
//
//        // Enter this token stream so that we re-lex the tokens.  Make sure to
//        // enable macro expansion, in case the token after the # is an identifier
//        // that is expanded.
//        EnterTokenStream(std::move(Toks), 2, false, /*IsReinject*/false);
//        return;
//    }
//
//    // If we reached here, the preprocessing token is not valid!
//    // Start suggesting if a similar directive found.
//    Diag(Result, diag::err_pp_invalid_directive) << 0;
//
//    // Read the rest of the PP line.
//    DiscardUntilEndOfDirective();
//
//    // Okay, we're done parsing the directive.
//}