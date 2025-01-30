#include <bson/format.h>
#include <bson/iterator.h>
#include <bson/types.h>

#include <fmt/base.h>

mlib_diagnostic_push();
mlib_gcc_warning_disable("-Wstringop-overflow");
#include <fmt/chrono.h>
mlib_diagnostic_pop();

namespace {

struct mlib_os_writebuf {
    mlib_ostream out;

    // Small buffer to hold chars pending flush
    char smallbuf[512] = {0};
    // number of chars in the smallbuf
    size_t nbuf = 0;
    // Add a char to the output
    void put(char c) {
        smallbuf[nbuf] = c;
        ++nbuf;
        if (nbuf == sizeof smallbuf) {
            // Buffer is full. Send it
            flush();
        }
    }
    // Write pending output through the stream abstraction.
    void flush() {
        mlib_write(out, std::string_view(smallbuf, nbuf));
        nbuf = 0;
    }
};

// Output iterator that writes to an mos_wr_buf
struct writer_iter {
    mlib_os_writebuf* into;

    writer_iter& operator++() noexcept { return *this; }
    writer_iter  operator++(int) noexcept { return *this; }
    writer_iter& operator*() noexcept { return *this; }
    void         operator=(char c) noexcept { into->put(c); }
};

template <typename O>
struct bson_writer {
    O                _output;
    bson_fmt_options _opts;
    unsigned         depth = 0;

    bool multiline() { return this->_opts.nested_indent != 0; }

    bson_writer nested() {
        auto dup = *this;
        dup.depth++;
        return dup;
    }
    void add_newline() {
        write("\n");
        add_hspace(this->_opts.subsequent_indent);
        add_hspace(this->_opts.nested_indent * this->depth);
    }

    template <typename... Args>
    void write(fmt::format_string<Args...> fstr, Args&&... args) {
        fmt::format_to(_output, fmt::runtime(fstr), args...);
    }

    void add_hspace(std::size_t count) {
        const auto spaces = "            ";
        while (count) {
            auto len = (std::min)(strlen(spaces), count);
            write("{}", std::string_view(spaces, len));
            count -= len;
        }
    }

    void top_write(bson::view doc) {
        add_hspace(_opts.initial_indent);
        write_value(doc);
    }

    void write_doc(bson::view doc) {
        write("{{");
        auto iter = doc.begin();
        if (iter == doc.end()) {
            // Empty doc. No line break
            write(" }}");
            return;
        }
        if (std::next(iter) == doc.end() && iter->type() != bson_type_document
            && iter->type() != bson_type_array) {
            // Only one value
            write(" {:?}: ", iter->key());
            write_value(*iter);
            write(" }}");
            return;
        }
        if (this->multiline()) {
            for (auto ref : doc) {
                this->add_newline();
                add_hspace(this->_opts.nested_indent);
                write("{:?}: ", ref.key());
                this->nested().write_value(ref);
                write(",");
            }
            this->add_newline();
            write("}}");
        } else {
            while (iter != doc.end()) {
                write(" {:?}: ", iter->key());
                this->write_value(*iter);
                ++iter;
                if (iter != doc.end()) {
                    write(",");
                }
            }
            write(" }}");
        }
    }

    void write_value(bson_array_view arr) {
        write("[");
        auto iter = arr.begin();
        if (iter == arr.end()) {
            write("]");
            return;
        }
        if (this->multiline()) {
            for (auto ref : arr) {
                this->add_newline();
                add_hspace(this->_opts.nested_indent);
                this->nested().write_value(ref);
                write(",");
            }
            this->add_newline();
            write("]");
        } else {
            while (iter != arr.end()) {
                write(" ", iter->key());
                this->write_value(*iter);
                ++iter;
                if (iter != arr.end()) {
                    write(",");
                }
            }
            write(" ]");
        }
    }

    void write_value(bson_iterator::reference ref) {
        ref.value().visit([&](auto x) { this->write_value(x); });
    }

    void write_value(std::string_view sv) { write("{:?}", sv); }
    void write_value(bson_symbol_view s) { write("Symbol({:?})", std::string_view(s.utf8)); }

    auto _as_formattable_time_point(std::int64_t utc_ms_offset) {
#if FMT_USE_UTC_TIME
        // C++20 UTC time point is supported
        return std::chrono::utc_clock::time_point(std::chrono::milliseconds(utc_ms_offset));
#else
        // Fall back to the system clock. This is not certain to give the correct answer,
        // but we're just logging, not saving the world
        return std::chrono::system_clock::time_point(std::chrono::milliseconds(utc_ms_offset));
#endif
    }

    void write_value(::bson_datetime dt) {
        auto tp = _as_formattable_time_point(dt.utc_ms_offset);
        write("Datetime⟨{:%c}⟩", tp);
    }
    void write_value(::bson_timestamp ts) {
        auto tp = _as_formattable_time_point(ts.utc_sec_offset);
        write("Timestamp(⟨{:%c}⟩ : {})", tp, ts.increment);
    }
    void write_value(::bson_code_view c) { write("Code({:?})", std::string_view(c.utf8)); }
    void write_value(::bson_decimal128) { write("[[Unimplemented: Decimal128 printing]]"); }

    void write_value(bson_eod) {
        assert(false && "Should never be called. Tried to format a phony EOD element.");
    }
    void write_value(bool b) { write("{}", b); }
    void write_value(std::int32_t i) { write("{}:i32", i); }
    void write_value(std::int64_t i) { write("{}:i64", i); }
    void write_value(double i) { write("{}:f64", i); }
    void write_value(bson::null) { write("null"); }
    void write_value(bson::undefined) { write("undefined"); }
    void write_value(bson::minkey) { write("[[min key]]"); }
    void write_value(bson::maxkey) { write("[[max key]]"); }

    void write_value(bson_view subdoc) { this->write_doc(subdoc); }
    void write_value(bson_dbpointer_view dbp) {
        write("DBPointer(\"{}\", ", std::string_view(dbp.collection));
        write_value(dbp.object_id);
        write(")");
    }
    void write_value(bson_oid oid) {
        write("ObjectID(");
        print_bytes(oid.bytes);
        write(")");
    }
    void print_bytes(auto&& seq) {
        for (auto n : seq) {
            write("{:0>2x}", std::uint8_t(std::byte(n)));
        }
    }
    void write_value(bson_regex_view rx) {
        write("/{}/{}", std::string_view(rx.regex), std::string_view(rx.options));
    }
    void write_value(bson_binary_view bin) {
        write("Binary(subtype {}, bytes 0x", bin.subtype);
        print_bytes(std::ranges::subrange(bin.data, bin.data + bin.data_len));
        write(")");
    }
};

}  // namespace

void(bson_write_repr)(mlib_ostream out, bson_view doc, bson_fmt_options const* opts) noexcept {
    mlib_os_writebuf         o{out};
    bson_writer<writer_iter> wr{writer_iter{&o},
                                opts ? *opts
                                     : bson_fmt_options{
                                           .initial_indent    = 0,
                                           .subsequent_indent = 0,
                                           .nested_indent     = 2,
                                       }};
    wr.top_write(doc);
    o.flush();
}
