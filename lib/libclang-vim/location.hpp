#if !defined LIBCLANG_VIM_LOCATION_HPP_INCLUDED
#define      LIBCLANG_VIM_LOCATION_HPP_INCLUDED

#include <tuple>
#include <string>
#include <cstdio>
#include <algorithm>


#include <clang-c/Index.h>

#include "helpers.hpp"
#include "stringizers.hpp"

namespace libclang_vim {

namespace detail {

    template<class Predicate>
    CXCursor search_AST_upward(CXCursor cursor, Predicate const& predicate)
    {
        while (!clang_isInvalid(clang_getCursorKind(cursor))) {
            if (predicate(cursor)){
                return cursor;
            }
            cursor = clang_getCursorSemanticParent(cursor);
        }
        return clang_getNullCursor();
    }

    template<class LocationTuple, class Predicate>
    auto invoke_at_specific_location_with(
            LocationTuple const& location_tuple,
            Predicate const& predicate,
            char const* argv[] = {},
            int const argc = 0
        ) -> char const*
    {
        static std::string vimson;
        char const* file_name = std::get<2>(location_tuple).c_str();

        CXIndex index = clang_createIndex(/*excludeDeclsFromPCH*/ 1, /*displayDiagnostics*/0);
        CXTranslationUnit translation_unit = clang_parseTranslationUnit(index, file_name, argv, argc, NULL, 0, CXTranslationUnit_Incomplete);
        if (translation_unit == NULL) {
            clang_disposeIndex(index);
            return "{}";
        }

        CXFile const file = clang_getFile(translation_unit, file_name);
        auto const location = clang_getLocation(translation_unit, file, std::get<0>(location_tuple), std::get<1>(location_tuple));
        CXCursor const cursor = clang_getCursor(translation_unit, location);

        vimson = predicate(cursor);

        clang_disposeTranslationUnit(translation_unit);
        clang_disposeIndex(index);

        return vimson.c_str();
    }

} // namespace detail

template<class LocationTuple, class Predicate>
auto get_extent(
        LocationTuple const& location_tuple,
        Predicate const& predicate,
        char const* argv[] = {},
        int const argc = 0
    ) -> char const*
{
    return detail::invoke_at_specific_location_with(
                location_tuple,
                [&predicate](CXCursor const& c) -> std::string {
                    CXCursor const rc = detail::search_AST_upward(c, predicate);
                    if (clang_Cursor_isNull(rc)) {
                        return "{}";
                    } else {
                        return "{" + stringize_extent(rc) + "}";
                    }
                },
                argv, argc);
};

template<class LocationTuple, class JumpFunc>
inline auto get_related_node_of(
        LocationTuple const& location_tuple,
        JumpFunc const& predicate,
        char const* argv[] = {},
        int const argc = 0
    ) -> char const*
{
    return detail::invoke_at_specific_location_with(
                location_tuple,
                [&predicate](CXCursor const& c) -> std::string {
                    CXCursor const rc = predicate(c);
                    if (clang_isInvalid(clang_getCursorKind(rc))) {
                        return "{}";
                    } else {
                        return "{" + stringize_cursor(rc, clang_getCursorSemanticParent(rc)) + "}";
                    }
                },
                argv, argc);
}

template<class LocationTuple, class JumpFunc>
inline auto get_type_related_to(
        LocationTuple const& location_tuple,
        JumpFunc const& predicate,
        char const* argv[] = {},
        int const argc = 0
    ) -> char const*
{
    return detail::invoke_at_specific_location_with(
            location_tuple,
            [&predicate](CXCursor const& c) -> std::string {
                CXType const type = predicate(clang_getCursorType(c));
                if (type.kind == CXType_Invalid) {
                    return "{}";
                } else {
                    return "{" + stringize_type(type)+ "}";
                }
            },
            argv, argc);
}

template<class LocationTuple>
auto get_all_extents(
        LocationTuple const& location_tuple,
        char const* argv[] = {},
        int const argc = 0
    ) -> char const*
{
    static std::string vimson;
    vimson = "";
    char const* file_name = std::get<2>(location_tuple).c_str();

    CXIndex index = clang_createIndex(/*excludeDeclsFromPCH*/ 1, /*displayDiagnostics*/0);
    CXTranslationUnit translation_unit = clang_parseTranslationUnit(index, file_name, argv, argc, NULL, 0, CXTranslationUnit_Incomplete);
    if (translation_unit == NULL) {
        clang_disposeIndex(index);
        return "[]";
    }

    CXFile const file = clang_getFile(translation_unit, file_name);
    auto const location = clang_getLocation(translation_unit, file, std::get<0>(location_tuple), std::get<1>(location_tuple));
    CXCursor cursor = clang_getCursor(translation_unit, location);
    auto const kind_name = owned(clang_getCursorKindSpelling(clang_getCursorKind(cursor)));
    vimson += "{" + stringize_key_value("kind", kind_name) + stringize_extent(cursor) + "},";

    bool already_pass_expression = false, already_pass_statement = false;
    while (!clang_isInvalid(clang_getCursorKind(cursor))) {
        if ( is_class_decl(cursor) ||
             is_function_decl(cursor) ||
             clang_getCursorKind(cursor) == CXCursor_Namespace ||
             (!already_pass_expression && clang_isExpression(clang_getCursorKind(cursor))) ||
             (!already_pass_statement && clang_isStatement(clang_getCursorKind(cursor)))
           ) {
            auto const kind_name = owned(clang_getCursorKindSpelling(clang_getCursorKind(cursor)));
            vimson += "{" + stringize_key_value("kind", kind_name) + stringize_extent(cursor) + "},";
        }
        cursor = clang_getCursorSemanticParent(cursor);
    }

    vimson = "[" + vimson + "]";

    clang_disposeTranslationUnit(translation_unit);
    clang_disposeIndex(index);

    return vimson.c_str();
}

} // namespace libclang_vim

#endif    // LIBCLANG_VIM_LOCATION_HPP_INCLUDED
