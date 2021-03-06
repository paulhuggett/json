/// \file dom_types.cpp

#include "dom_types.hpp"
#include <algorithm>

namespace json {
    namespace value {

        dom_element::~dom_element () noexcept = default;
        null_value::~null_value () noexcept = default;
        number_base::~number_base () noexcept = default;
        number_double::~number_double () noexcept = default;
        number_long::~number_long () noexcept = default;
        string_value::~string_value () noexcept = default;

        std::ostream & boolean_value::write (std::ostream & os) const {
            return os << (b_ ? "true" : "false");
        }
        std::ostream & array_value::write (std::ostream & os) const {
            os << '[';
            auto separator = "";
            for (auto const & v : v_) {
                os << separator;
                v->write (os);
                separator = ", ";
            }
            return os << ']';
        }
        std::ostream & object_value::write (std::ostream & os) const {
            os << '{';
            char const * separator = "";
            for (auto const & v : v_) {
                os << separator << '"' << v.first << "\": ";
                v.second->write (os);
                separator = ", ";
            }
            return os << '}';
        }

    } // namespace value


    void yaml_output::begin_array () { out_.emplace (nullptr); }
    void yaml_output::end_array () {
        value::array_value::container_type content;
        for (;;) {
            auto v = out_.top ();
            out_.pop ();
            if (!v) {
                break;
            }
            content.push_back (std::move (v));
        }
        std::reverse (std::begin (content), std::end (content));
        out_.emplace (new value::array_value (content));
    }

    void yaml_output::begin_object () { out_.emplace (nullptr); }
    void yaml_output::end_object () {
        auto object = std::make_shared<value::object_value> ();
        for (;;) {
            auto value = out_.top ();
            out_.pop ();
            if (!value) {
                break;
            }

            auto key = out_.top ();
            out_.pop ();
            assert (key);
            auto key_str = key->as_string ();
            assert (key_str);
            object->insert (key_str->get (), value);
        }
        out_.emplace (std::move (object));
    }
} // namespace json
