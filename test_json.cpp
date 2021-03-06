#include "json.hpp"
#include <stack>
#include "dom_types.hpp"
#include "gmock/gmock.h"
namespace {

    class json_callbacks_base {
    public:
        virtual ~json_callbacks_base () {}

        virtual void string_value (std::string const &) {}
        virtual void integer_value (long) {}
        virtual void float_value (double) {}
        virtual void boolean_value (bool) {}
        virtual void null_value () {}
        virtual void begin_array () {}
        virtual void end_array () {}
        virtual void begin_object () {}
        virtual void end_object () {}
    };

    class mock_json_callbacks : public json_callbacks_base {
    public:
        MOCK_METHOD1 (string_value, void(std::string const &));
        MOCK_METHOD1 (integer_value, void(long));
        MOCK_METHOD1 (float_value, void(double));
        MOCK_METHOD1 (boolean_value, void(bool));
        MOCK_METHOD0 (null_value, void());
        MOCK_METHOD0 (begin_array, void());
        MOCK_METHOD0 (end_array, void());
        MOCK_METHOD0 (begin_object, void());
        MOCK_METHOD0 (end_object, void());
    };

    template <typename T>
    class callbacks_proxy {
    public:
        using result_type = void;
        result_type result () {}

        explicit callbacks_proxy (T & original)
                : original_{original} {}
        callbacks_proxy (callbacks_proxy const &) = default;

        void string_value (std::string const & s) { original_.string_value (s); }
        void integer_value (long v) { original_.integer_value (v); }
        void float_value (double v) { original_.float_value (v); }
        void boolean_value (bool v) { original_.boolean_value (v); }
        void null_value () { original_.null_value (); }
        void begin_array () { original_.begin_array (); }
        void end_array () { original_.end_array (); }
        void begin_object () { original_.begin_object (); }
        void end_object () { original_.end_object (); }

    private:
        T & original_;
    };

    class null_callbacks {
    public:
        using result_type = void;
        result_type result () {}

        void string_value (std::string const &) {}
        void integer_value (long) {}
        void float_value (double v) {}
        void boolean_value (bool v) {}
        void null_value () {}
        void begin_array () {}
        void end_array () {}
        void begin_object () {}
        void end_object () {}
    };

    class Json : public ::testing::Test {
    protected:
        static std::shared_ptr<json::value::dom_element> parse (char const * src) {
            json::parser<json::yaml_output> p;
            std::shared_ptr<json::value::dom_element> v = p.parse (src);
            EXPECT_EQ (p.last_error (), std::make_error_code (json::error_code::none));
            return v;
        }

        static void check_error (char const * src, json::error_code err) {
            ASSERT_NE (err, json::error_code::none);
            json::parser<json::yaml_output> p;
            std::shared_ptr<json::value::dom_element> v = p.parse (src);
            EXPECT_EQ (v, nullptr);
            EXPECT_NE (p.last_error (), std::make_error_code (json::error_code::none));
        }
    };

} // end anonymous namespace

TEST_F (Json, Empty) {
    check_error ("", json::error_code::expected_token);
    check_error ("   \t    ", json::error_code::expected_token);
}

using ::testing::StrictMock;

TEST_F (Json, Null) {
    StrictMock<mock_json_callbacks> callbacks;
    callbacks_proxy<mock_json_callbacks> proxy (callbacks);
    EXPECT_CALL (callbacks, null_value ()).Times (1);

    json::parser<decltype (proxy)> p (proxy);
    p.parse (" null ");
}
TEST_F (Json, TwoKeywords) {
    json::parser<json::yaml_output> p;
    std::shared_ptr<json::value::dom_element> v = p.parse (" true false ");
    EXPECT_EQ (v, nullptr);
    EXPECT_EQ (p.last_error (), std::make_error_code (json::error_code::unexpected_extra_input));
}
TEST_F (Json, BadKeyword) {
    check_error ("nu", json::error_code::expected_token);
    check_error ("bad", json::error_code::expected_token);
    check_error ("fal", json::error_code::expected_token);
    check_error ("falsehood", json::error_code::unexpected_extra_input);
}


namespace {
    class JsonNumber : public Json {};
} // namespace

TEST_F (JsonNumber, MinusOnly) {
    this->check_error ("-", json::error_code::expected_digits);
}
TEST_F (JsonNumber, One) {
    StrictMock<mock_json_callbacks> callbacks;
    callbacks_proxy<mock_json_callbacks> proxy (callbacks);
    EXPECT_CALL (callbacks, integer_value (1L)).Times (1);

    json::parser<decltype (proxy)> p (proxy);
    p.parse (" 1 ");
}
TEST_F (JsonNumber, LeadingZero) {
    this->check_error ("01", json::error_code::unexpected_extra_input);
}
TEST_F (JsonNumber, AllDigits) {
    StrictMock<mock_json_callbacks> callbacks;
    callbacks_proxy<mock_json_callbacks> proxy (callbacks);
    EXPECT_CALL (callbacks, integer_value (1234567890L)).Times (1);

    json::parser<decltype (proxy)> p (proxy);
    p.parse ("1234567890");
}
TEST_F (JsonNumber, NegativeZero) {
    StrictMock<mock_json_callbacks> callbacks;
    callbacks_proxy<mock_json_callbacks> proxy (callbacks);
    EXPECT_CALL (callbacks, integer_value (0L)).Times (1);

    json::parser<decltype (proxy)> p (proxy);
    p.parse ("-0");
}
TEST_F (JsonNumber, NegativeOne) {
    StrictMock<mock_json_callbacks> callbacks;
    callbacks_proxy<mock_json_callbacks> proxy (callbacks);
    EXPECT_CALL (callbacks, integer_value (-1L)).Times (1);

    json::parser<decltype (proxy)> p (proxy);
    p.parse ("-1");
}
TEST_F (JsonNumber, NegativeOneLeadingZero) {
    this->check_error ("-01", json::error_code::unexpected_extra_input);
}
TEST_F (JsonNumber, RealUnderflow) {
    StrictMock<mock_json_callbacks> callbacks;
    callbacks_proxy<mock_json_callbacks> proxy (callbacks);
    json::parser<decltype (proxy)> p = json::make_parser (proxy);
    p.parse ("123e-10000000");
    EXPECT_EQ (p.last_error (), std::make_error_code (json::error_code::number_out_of_range));
}


namespace {

    class JsonBoolean : public Json {
    protected:
        void check_bool (char const * src, bool expected) {
            std::shared_ptr<json::value::dom_element> v = this->parse (src);
            ASSERT_NE (v, nullptr);
            auto b = v->as_boolean ();
            ASSERT_NE (b, nullptr);
            EXPECT_EQ (b->get (), expected);
        }
    };

} // end anonymous namespace

TEST_F (JsonBoolean, True) {
    char const * input = " true ";
    {
        StrictMock<mock_json_callbacks> callbacks;
        callbacks_proxy<mock_json_callbacks> proxy (callbacks);
        EXPECT_CALL (callbacks, boolean_value (true)).Times (1);

        json::parser<decltype (proxy)> p = json::make_parser (proxy);
        p.parse (input);
    }
    {
        json::parser<json::yaml_output> p2;
        p2.parse (input);

        std::shared_ptr<json::value::dom_element> resl = p2.callbacks ().result ();
        ASSERT_NE (resl, nullptr);

        auto b = resl->as_boolean ();
        ASSERT_NE (b, nullptr);
        EXPECT_TRUE (b->get ());
    }
}
TEST_F (JsonBoolean, False) {
    StrictMock<mock_json_callbacks> callbacks;
    callbacks_proxy<mock_json_callbacks> proxy (callbacks);
    EXPECT_CALL (callbacks, boolean_value (false)).Times (1);

    json::parser<decltype (proxy)> p = json::make_parser (proxy);
    p.parse (" false ");
}



namespace {

    class JsonString : public Json {
    protected:
        void check (char const * src, char const * expected) {
            ASSERT_NE (src, nullptr);
            ASSERT_NE (expected, nullptr);

            StrictMock<mock_json_callbacks> callbacks;
            callbacks_proxy<mock_json_callbacks> proxy (callbacks);
            EXPECT_CALL (callbacks, string_value (expected)).Times (1);

            json::parser<decltype (proxy)> p = json::make_parser (proxy);
            p.parse (src);
            EXPECT_EQ (p.last_error (), std::make_error_code (json::error_code::none));
        }
    };

} // namespace

TEST_F (JsonString, Simple) {
    this->check ("\"\"", "");
    this->check ("\"hello\"", "hello");
}
TEST_F (JsonString, Unterminated) {
    this->check_error ("\"hello", json::error_code::expected_close_quote);
}
TEST_F (JsonString, EscapeN) {
    this->check ("\"a\\n\"", "a\n");
}
TEST_F (JsonString, BadEscape) {
    this->check_error ("\"a\\qb\"", json::error_code::invalid_escape_char);
}
TEST_F (JsonString, BackslashQuoteUnterminated) {
    this->check_error ("\"a\\\"", json::error_code::expected_close_quote);
}
TEST_F (JsonString, TrailingBackslashUnterminated) {
    this->check_error ("\"a\\", json::error_code::invalid_escape_char);
}
TEST_F (JsonString, GCleffUtf8) {
    // Encoding for MUSICAL SYMBOL G CLEF (U+1D11E) expressed in UTF-8
    this->check ("\"\xF0\x9D\x84\x9E\"", "\xF0\x9D\x84\x9E");
}
TEST_F (JsonString, SlashUnicodeUpper) {
    this->check ("\"\\u002F\"", "/");
}
TEST_F (JsonString, TwoUtf16Chars) {
    // Encoding for TURNED AMPERSAND (U+214B) followed by KATAKANA LETTER SMALL A (u+30A1)
    // expressed as a pair of UTF-16 characters.
    this->check ("\"\\u214B\\u30A1\"", "\xE2\x85\x8B\xE3\x82\xA1");
}
TEST_F (JsonString, Utf16Surrogates) {
    // Encoding for MUSICAL SYMBOL G CLEF (U+1D11E) expressed as a UTF-16
    // surrogate pair.
    this->check ("\"\\uD834\\uDD1E\"", "\xF0\x9D\x84\x9E");
}
TEST_F (JsonString, Utf16HighWithNoLowSurrogate) {
    // UTF-16 high surrogate followed by non-surrogate UTF-16 hex code point.
    this->check_error ("\"\\uD834\\u30A1\"", json::error_code::bad_unicode_code_point);
}
TEST_F (JsonString, Utf16HighFollowedByUtf8Char) {
    // UTF-16 high surrogate followed by non-surrogate UTF-16 hex code point.
    this->check_error ("\"\\uD834!\"", json::error_code::bad_unicode_code_point);
}
TEST_F (JsonString, Utf16HighWithMissingLowSurrogate) {
    // Encoding for MUSICAL SYMBOL G CLEF (U+1D11E) expressed as a UTF-16
    // surrogate pair.
    this->check_error ("\"\\uDD1E\\u30A1\"", json::error_code::bad_unicode_code_point);
}
TEST_F (JsonString, ControlCharacter) {
    this->check_error ("\"\t\"", json::error_code::bad_unicode_code_point);

    this->check ("\"\\u0009\"", "\t");
}

TEST_F (JsonString, Utf16LowWithNoHighSurrogate) {
    // UTF-16 high surrogate followed by non-surrogate UTF-16 hex code point.
    this->check_error ("\"\\uD834\"", json::error_code::bad_unicode_code_point);
}
TEST_F (JsonString, SlashBadHexChar) {
    this->check_error ("\"\\u00xF\"", json::error_code::invalid_escape_char);
}
TEST_F (JsonString, PartialHexChar) {
    this->check_error ("\"\\u00", json::error_code::invalid_escape_char);
}



TEST (JsonArray, Empty) {
    StrictMock<mock_json_callbacks> callbacks;
    {
        ::testing::InSequence _;
        EXPECT_CALL (callbacks, begin_array ()).Times (1);
        EXPECT_CALL (callbacks, end_array ()).Times (1);
    }
    auto p = json::make_parser (callbacks_proxy<mock_json_callbacks> (callbacks));
    p.parse (" [ ] ");
    EXPECT_EQ (p.last_error (), std::make_error_code (json::error_code::none));
}

TEST (JsonArray, ArrayNoCloseBracket) {
    StrictMock<mock_json_callbacks> callbacks;
    EXPECT_CALL (callbacks, begin_array ()).Times (1);

    auto p = json::make_parser (callbacks_proxy<mock_json_callbacks> (callbacks));
    p.parse ("[");
    EXPECT_EQ (p.last_error (), std::make_error_code (json::error_code::expected_array_member));
}

TEST (JsonArray, SingleElement) {
    char const * input = "[ 1 ]";
    {
        StrictMock<mock_json_callbacks> callbacks;
        {
            ::testing::InSequence _;
            EXPECT_CALL (callbacks, begin_array ()).Times (1);
            EXPECT_CALL (callbacks, integer_value (1L)).Times (1);
            EXPECT_CALL (callbacks, end_array ()).Times (1);
        }
        auto p = json::make_parser (callbacks_proxy<mock_json_callbacks> (callbacks));
        p.parse (input);
        EXPECT_EQ (p.last_error (), std::make_error_code (json::error_code::none));
    }
    {
        json::parser<json::yaml_output> p;
        std::shared_ptr<json::value::dom_element> resl = p.parse (input);
        EXPECT_EQ (p.last_error (), std::make_error_code (json::error_code::none));

        ASSERT_NE (resl, nullptr);

        auto arr = resl->as_array ();
        ASSERT_NE (arr, nullptr);
        ASSERT_EQ (arr->size (), 1U);
        auto element = ((*arr)[0])->as_long ();
        ASSERT_NE (element, nullptr);
        EXPECT_EQ (element->get (), 1);
    }
}

TEST (JsonArray, SingleStringElement) {
    char const * input = "[\"a\"]";
    {
        StrictMock<mock_json_callbacks> callbacks;
        {
            ::testing::InSequence _;
            EXPECT_CALL (callbacks, begin_array ()).Times (1);
            EXPECT_CALL (callbacks, string_value ("a")).Times (1);
            EXPECT_CALL (callbacks, end_array ()).Times (1);
        }
        auto p = json::make_parser (callbacks_proxy<mock_json_callbacks> (callbacks));
        p.parse (input);
        EXPECT_EQ (p.last_error (), std::make_error_code (json::error_code::none));
    }
    {
        json::parser<json::yaml_output> p;
        p.parse (input);
        EXPECT_EQ (p.last_error (), std::make_error_code (json::error_code::none));

        std::shared_ptr<json::value::dom_element> v = p.callbacks ().result ();
        ASSERT_NE (v, nullptr);

        auto arr = v->as_array ();
        ASSERT_NE (arr, nullptr);
        ASSERT_EQ (arr->size (), 1U);
        auto element = ((*arr)[0])->as_string ();
        ASSERT_NE (element, nullptr);
        EXPECT_EQ (element->get (), "a");
    }
}

TEST (JsonArray, ZeroExpPlus1) {
    char const * input = "[0e+1]";

    {
        StrictMock<mock_json_callbacks> callbacks;
        {
            ::testing::InSequence _;
            EXPECT_CALL (callbacks, begin_array ()).Times (1);
            EXPECT_CALL (callbacks, float_value (::testing::DoubleEq (0.0))).Times (1);
            EXPECT_CALL (callbacks, end_array ()).Times (1);
        }
        auto p = json::make_parser (callbacks_proxy<mock_json_callbacks> (callbacks));
        p.parse (input);
        EXPECT_EQ (p.last_error (), std::make_error_code (json::error_code::none));
    }
    {
        json::parser<json::yaml_output> p;
        std::shared_ptr<json::value::dom_element> v = p.parse (input);
        EXPECT_EQ (p.last_error (), std::make_error_code (json::error_code::none));
        ASSERT_NE (v, nullptr);

        auto arr = v->as_array ();
        ASSERT_NE (arr, nullptr);
        ASSERT_EQ (arr->size (), 1U);
        auto element = ((*arr)[0])->as_double ();
        ASSERT_NE (element, nullptr);
        EXPECT_EQ (element->get (), 0);
    }
}
TEST (JsonArray, MinusZero) {
    json::parser<json::yaml_output> p;
    std::shared_ptr<json::value::dom_element> v = p.parse ("[-0]");
    EXPECT_EQ (p.last_error (), std::make_error_code (json::error_code::none));
    ASSERT_NE (v, nullptr);

    auto arr = v->as_array ();
    ASSERT_NE (arr, nullptr);
    ASSERT_EQ (arr->size (), 1U);
    auto element = ((*arr)[0])->as_long ();
    ASSERT_NE (element, nullptr);
    EXPECT_EQ (element->get (), 0);
}


TEST (JsonArray, TwoElements) {
    json::parser<json::yaml_output> p;
    std::shared_ptr<json::value::dom_element> v = p.parse ("[ 1 , \"hello\" ]");
    EXPECT_EQ (p.last_error (), std::make_error_code (json::error_code::none));
    ASSERT_NE (v, nullptr);

    auto arr = v->as_array ();
    ASSERT_NE (arr, nullptr);
    ASSERT_EQ (arr->size (), 2U);

    auto element0 = ((*arr)[0])->as_long ();
    ASSERT_NE (element0, nullptr);
    EXPECT_EQ (element0->get (), 1);

    auto element1 = ((*arr)[1])->as_string ();
    ASSERT_NE (element1, nullptr);
    EXPECT_EQ (element1->get (), "hello");
}

TEST (JsonArray, TrailingComma) {
    {
        json::parser<json::yaml_output> p;
        std::shared_ptr<json::value::dom_element> v = p.parse ("[,");
        EXPECT_EQ (v, nullptr);
        EXPECT_EQ (p.last_error (), std::make_error_code (json::error_code::expected_token));
    }
    {
        json::parser<json::yaml_output> p;
        std::shared_ptr<json::value::dom_element> v = p.parse ("[,]");
        EXPECT_EQ (v, nullptr);
        EXPECT_EQ (p.last_error (), std::make_error_code (json::error_code::expected_token));
    }
    {
        json::parser<json::yaml_output> p;
        std::shared_ptr<json::value::dom_element> v = p.parse ("[\"\",]");
        EXPECT_EQ (v, nullptr);
        EXPECT_EQ (p.last_error (), std::make_error_code (json::error_code::expected_token));
    }
}
TEST (JsonArray, TrailingCommaWithExtraText) {
    json::parser<json::yaml_output> p;
    std::shared_ptr<json::value::dom_element> v = p.parse ("[,1");
    ASSERT_EQ (v, nullptr);
    EXPECT_EQ (p.last_error (), std::make_error_code (json::error_code::expected_token));
}
TEST (JsonArray, ArraySingleElementComma) {
    json::parser<json::yaml_output> p;
    std::shared_ptr<json::value::dom_element> v = p.parse ("[1,");
    ASSERT_EQ (v, nullptr);
    EXPECT_EQ (p.last_error (), std::make_error_code (json::error_code::expected_array_member));
}
TEST (JsonArray, Nested) {
    json::parser<json::yaml_output> p;
    std::shared_ptr<json::value::dom_element> v = p.parse ("[[no");
    ASSERT_EQ (v, nullptr);
    EXPECT_EQ (p.last_error (), std::make_error_code (json::error_code::unrecognized_token));
}
TEST (JsonArray, MissingComma) {
    json::parser<json::yaml_output> p;
    std::shared_ptr<json::value::dom_element> v = p.parse ("[1 true]");
    ASSERT_EQ (v, nullptr);
    EXPECT_EQ (p.last_error (), std::make_error_code (json::error_code::expected_array_member));
}
TEST (JsonArray, ExtraComma) {
    json::parser<json::yaml_output> p;
    std::shared_ptr<json::value::dom_element> v = p.parse ("[1,,2]");
    ASSERT_EQ (v, nullptr);
    EXPECT_EQ (p.last_error (), std::make_error_code (json::error_code::expected_token));
}
TEST (JsonArray, SimpleFloat) {
    json::parser<json::yaml_output> p;
    std::shared_ptr<json::value::dom_element> v = p.parse ("[1.234]");
    ASSERT_NE (v, nullptr);

    auto arr = v->as_array ();
    ASSERT_NE (arr, nullptr);
    ASSERT_EQ (arr->size (), 1U);

    auto element0 = ((*arr)[0])->as_double ();
    ASSERT_NE (element0, nullptr);
    EXPECT_DOUBLE_EQ (element0->get (), 1.234);
}



TEST (JsonObject, Empty) {
    json::parser<json::yaml_output> p;
    std::shared_ptr<json::value::dom_element> v = p.parse ("{}");
    auto obj = v->as_object ();
    ASSERT_NE (obj, nullptr);
    EXPECT_EQ (obj->size (), 0U);
}
TEST (JsonObject, SingleKvp) {
    json::parser<json::yaml_output> p;
    std::shared_ptr<json::value::dom_element> v = p.parse ("{\"a\":1}");
    ASSERT_NE (v, nullptr);
    auto obj = v->as_object ();
    ASSERT_NE (obj, nullptr);
    EXPECT_EQ (obj->size (), 1U);

    auto pos = obj->find ("a");
    ASSERT_NE (pos, obj->end ());
    EXPECT_EQ (pos->first, "a");

    auto value = pos->second->as_long ();
    ASSERT_NE (value, nullptr);
    EXPECT_EQ (value->get (), 1);
}
TEST (JsonObject, TwoKvps) {
    char const * input = "{\"a\":1, \"b\" : true }";
    {
        StrictMock<mock_json_callbacks> callbacks;
        {
            ::testing::InSequence _;
            EXPECT_CALL (callbacks, begin_object ()).Times (1);
            EXPECT_CALL (callbacks, string_value ("a")).Times (1);
            EXPECT_CALL (callbacks, integer_value (1)).Times (1);
            EXPECT_CALL (callbacks, string_value ("b")).Times (1);
            EXPECT_CALL (callbacks, boolean_value (true)).Times (1);
            EXPECT_CALL (callbacks, end_object ()).Times (1);
        }
        auto p = json::make_parser (callbacks_proxy<mock_json_callbacks> (callbacks));
        p.parse (input);
        EXPECT_EQ (p.last_error (), std::make_error_code (json::error_code::none));
    }
    {
        json::parser<json::yaml_output> p;
        std::shared_ptr<json::value::dom_element> v = p.parse (input);
        ASSERT_NE (v, nullptr);
        auto obj = v->as_object ();
        ASSERT_NE (obj, nullptr);
        EXPECT_EQ (obj->size (), 2U);
        {
            auto pos_a = obj->find ("a");
            ASSERT_NE (pos_a, obj->end ());

            EXPECT_EQ (pos_a->first, "a");
            auto value = pos_a->second->as_long ();
            ASSERT_NE (value, nullptr);
            EXPECT_EQ (value->get (), 1);
        }
        {
            auto pos_b = obj->find ("b");
            ASSERT_NE (pos_b, obj->end ());

            EXPECT_EQ (pos_b->first, "b");
            auto value = pos_b->second->as_boolean ();
            ASSERT_NE (value, nullptr);
            EXPECT_EQ (value->get (), true);
        }
    }
}
TEST (JsonObject, ArrayValue) {
    json::parser<json::yaml_output> p;
    std::shared_ptr<json::value::dom_element> v = p.parse ("{\"a\": [1,2]}");
    ASSERT_NE (v, nullptr);
    auto obj = v->as_object ();
    ASSERT_NE (obj, nullptr);
    EXPECT_EQ (obj->size (), 1U);

    auto pos = obj->find ("a");
    ASSERT_NE (pos, obj->end ());

    auto value = pos->second->as_array ();
    ASSERT_NE (value, nullptr);
    EXPECT_EQ (value->size (), 2U);
    {
        auto v0 = ((*value)[0])->as_long ();
        ASSERT_NE (v0, nullptr);
        ASSERT_EQ (v0->get (), 1);
    }
    {
        auto v1 = ((*value)[1])->as_long ();
        ASSERT_NE (v1, nullptr);
        ASSERT_EQ (v1->get (), 2);
    }
}
TEST (JsonObject, TrailingComma) {
    json::parser<json::yaml_output> p;
    std::shared_ptr<json::value::dom_element> v = p.parse ("{\"a\":1,}");
    EXPECT_EQ (v, nullptr);
    EXPECT_EQ (p.last_error (), std::make_error_code (json::error_code::expected_token));
}
TEST (JsonObject, MissingComma) {
    json::parser<json::yaml_output> p;
    std::shared_ptr<json::value::dom_element> v = p.parse ("{\"a\":1 \"b\":1}");
    EXPECT_EQ (v, nullptr);
    EXPECT_EQ (p.last_error (), std::make_error_code (json::error_code::expected_object_member));
}
TEST (JsonObject, ExtraComma) {
    json::parser<json::yaml_output> p;
    std::shared_ptr<json::value::dom_element> v = p.parse ("{\"a\":1,,\"b\":1}");
    EXPECT_EQ (v, nullptr);
    EXPECT_EQ (p.last_error (), std::make_error_code (json::error_code::expected_token));
}
TEST (JsonObject, KeyIsNotString) {
    json::parser<json::yaml_output> p;
    p.parse ("{{}:{}}");
    EXPECT_EQ (p.last_error (), std::make_error_code (json::error_code::expected_string));
}
TEST (JsonObject, BadNestedObject) {
    json::parser<json::yaml_output> p;
    std::shared_ptr<json::value::dom_element> v = p.parse ("{\"a\":nu}");
    EXPECT_EQ (v, nullptr);
    EXPECT_EQ (p.last_error (), std::make_error_code (json::error_code::unrecognized_token));
}
