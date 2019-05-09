#include "stringizers.hpp"

std::string libclang_vim::stringize_spell(CXCursor const& cursor) {
    cxstring_ptr spell = clang_getCursorSpelling(cursor);
    return stringize_key_value("spell", spell);
}

std::string libclang_vim::stringize_extra_type_info(CXType const& type) {
    std::string result;

    if (clang_isConstQualifiedType(type)) {
        result += "'is_const_qualified':1,";
    }
    if (clang_isVolatileQualifiedType(type)) {
        result += "'is_volatile_qualified':1,";
    }
    if (clang_isRestrictQualifiedType(type)) {
        result += "'is_restrict_qualified':1,";
    }
    if (clang_isPODType(type)) {
        result += "'is_POD_type':1,";
    }

    auto const ref_qualified = clang_Type_getCXXRefQualifier(type);
    switch (ref_qualified) {
    case CXRefQualifier_LValue:
        result += "'is_lvalue':1,";
        break;
    case CXRefQualifier_RValue:
        result += "'is_rvalue':1,";
        break;
    case CXRefQualifier_None:
        break;
    }

    // Note: The information about calling convention is really needed?
    //       If you want the information, please let me know that.
    //
    // auto const calling_convention = clang_getFunctionTypeCallingConv(type);
    // switch (calling_convention) {
    // ...

    return result;
}

std::string libclang_vim::stringize_type(CXType const& type) {
    CXTypeKind const type_kind = type.kind;
    cxstring_ptr type_name = clang_getTypeSpelling(type);
    cxstring_ptr type_kind_name = clang_getTypeKindSpelling(type_kind);
    
    return stringize_key_value("type", type_name) +
           stringize_key_value("type_kind", type_kind_name) +
           stringize_extra_type_info(type);
}

std::string libclang_vim::stringize_linkage_kind(CXLinkageKind const& linkage) {
    switch (linkage) {
    case CXLinkage_Invalid:
        return "";
    case CXLinkage_NoLinkage:
        return "Nolinkage";
    case CXLinkage_Internal:
        return "Internal";
    case CXLinkage_UniqueExternal:
        return "UniqueExternal";
    case CXLinkage_External:
        return "External";
    }
}

std::string libclang_vim::stringize_linkage(CXCursor const& cursor) {
    auto const linkage_kind_name =
        stringize_linkage_kind(clang_getCursorLinkage(cursor));
    return stringize_key_value("linkage", linkage_kind_name);
}

std::string libclang_vim::stringize_parent(CXCursor const& cursor,
                                           CXCursor const& parent) {
    auto const semantic_parent = clang_getCursorSemanticParent(cursor);
    auto const lexical_parent = clang_getCursorLexicalParent(cursor);
    cxstring_ptr parent_name = clang_getCursorSpelling(parent);
    cxstring_ptr semantic_parent_name =
        clang_getCursorSpelling(semantic_parent);
    cxstring_ptr lexical_parent_name = clang_getCursorSpelling(lexical_parent);

    return stringize_key_value("parent", parent_name) +
           stringize_key_value("semantic_parent", semantic_parent_name) +
           stringize_key_value("lexical_parent", lexical_parent_name);
}

std::string libclang_vim::stringize_location(CXSourceLocation const& location) {
    CXFile file;
    unsigned int line, column, offset;
    clang_getSpellingLocation(location, &file, &line, &column, &offset);
    cxstring_ptr file_name = clang_getFileName(file);

    return "'line':" + std::to_string(line) + ",'column':" +
           std::to_string(column) + ",'offset':" + std::to_string(offset) +
           ',' + stringize_key_value("file", file_name);
}

std::string libclang_vim::stringize_cursor_location(CXCursor const& cursor) {
    CXSourceLocation const location = clang_getCursorLocation(cursor);
    return stringize_location(location);
}

std::string libclang_vim::stringize_cursor_kind_type(CXCursorKind const& kind) {
    if (clang_isAttribute(kind))
        return "Attribute";
    if (clang_isDeclaration(kind))
        return "Declaration";
    if (clang_isExpression(kind))
        return "Expression";
    if (clang_isPreprocessing(kind))
        return "Preprocessing";
    if (clang_isReference(kind))
        return "Reference";
    if (clang_isStatement(kind))
        return "Statement";
    if (clang_isTranslationUnit(kind))
        return "TranslationUnit";
    if (clang_isUnexposed(kind))
        return "Unexposed";
    if (clang_isInvalid(kind))
        return "";
    return "Unknown";
}

std::string libclang_vim::stringize_cursor_extra_info(CXCursor const& cursor) {
    std::string result;

    if (clang_isCursorDefinition(cursor)) {
        result += "'is_definition':1,";
    }
    if (clang_Cursor_isDynamicCall(cursor)) {
        result += "'is_dynamic_call':1,";
    }
    if (clang_Cursor_isVariadic(cursor)) {
        result += "'is_variadic':1,";
    }
    if (clang_CXXMethod_isVirtual(cursor)) {
        result += "'is_virtual_member_function':1,";
    }
    if (clang_CXXMethod_isPureVirtual(cursor)) {
        result += "'is_pure_virtual_member_function':1,";
    }
    if (clang_CXXMethod_isStatic(cursor)) {
        result += "'is_static_member_function':1,";
    }

    auto const access_specifier = clang_getCXXAccessSpecifier(cursor);
    switch (access_specifier) {
    case CX_CXXPublic:
        result += "'access_specifier':'public',";
        break;
    case CX_CXXPrivate:
        result += "'access_specifier':'private',";
        break;
    case CX_CXXProtected:
        result += "'access_specifier':'protected',";
        break;
    case CX_CXXInvalidAccessSpecifier:
        break;
    }

    return result;
}

std::string libclang_vim::stringize_cursor_kind(CXCursor const& cursor) {
    CXCursorKind const kind = clang_getCursorKind(cursor);
    cxstring_ptr kind_name = clang_getCursorKindSpelling(kind);
    auto const kind_type_name = stringize_cursor_kind_type(kind);

    std::string res_str = "";

    CXTranslationUnit tu = clang_Cursor_getTranslationUnit(cursor);

    const auto* kstring = clang_getCString(kind_name);
    std::string kind_str = "";
    if (kstring && std::strcmp(kstring, "") != 0) {
        kind_str = std::string{kstring};
    }

    if (kind == CXCursor_IntegerLiteral || kind == CXCursor_FloatingLiteral
        || kind == CXCursor_CharacterLiteral || kind == CXCursor_StringLiteral
        || kind == CXCursor_FixedPointLiteral || kind == CXCursor_ImaginaryLiteral)
    {
        CXSourceRange range = clang_getCursorExtent(cursor);
        CXToken *tokens = 0;
        unsigned int nTokens = 0;
        clang_tokenize(tu, range, &tokens, &nTokens);
        for (unsigned int i = 0; i < nTokens; i++)
        {
            cxstring_ptr spelling = clang_getTokenSpelling(tu, tokens[i]);
            const auto*  s = clang_getCString(spelling);
            if (s && std::strcmp(s, "") != 0) {
                res_str = "'value': '" + std::string{s} + "',";
            }
            //clang_disposeString(spelling);
        }
        clang_disposeTokens(tu, tokens, nTokens);
    }

    return stringize_key_value("kind", kind_name) +
        res_str + 
        (kind_type_name.empty() ? std::string{} : ("'kind_type':'" +
                                                      kind_type_name + "',")) +
           stringize_cursor_extra_info(cursor);
}

std::string libclang_vim::stringize_included_file(CXCursor const& cursor) {
    CXFile included_file = clang_getIncludedFile(cursor);
    if (included_file == nullptr) {
        return "";
    }

    cxstring_ptr included_file_name = clang_getFileName(included_file);
    return std::string("'included_file':'") + to_c_str(included_file_name) +
           "',";
}

std::string libclang_vim::stringize_cursor(CXCursor const& cursor,
                                           CXCursor const& parent) {
    return stringize_spell(cursor) +
           stringize_type(clang_getCursorType(cursor)) +
           stringize_linkage(cursor) + stringize_parent(cursor, parent) +
           stringize_cursor_location(cursor) + stringize_cursor_kind(cursor) +
           stringize_end(cursor) +   
           stringize_included_file(cursor);
}

std::string libclang_vim::stringize_range(CXSourceRange const& range) {
    if (clang_Range_isNull(range)) {
        return "";
    }
    return "'range':{'start':{" +
           stringize_location(clang_getRangeStart(range)) + "},'end':{" +
           stringize_location(clang_getRangeEnd(range)) + "}},";
}

std::string libclang_vim::stringize_end(CXCursor const& cursor) {
    auto const r = clang_getCursorExtent(cursor);
    if (clang_Range_isNull(r))
        return "";
    return "'start':{" + stringize_location(clang_getRangeStart(r)) +
           "},'end':{" + stringize_location(clang_getRangeEnd(r)) + "},";
}

std::string libclang_vim::stringize_extent(CXCursor const& cursor) {
    auto const r = clang_getCursorExtent(cursor);
    if (clang_Range_isNull(r))
        return "";
    return "'start':{" + stringize_location(clang_getRangeStart(r)) +
           "},'end':{" + stringize_location(clang_getRangeEnd(r)) + "}";
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
