#if !defined TCPIP_ROTO_SERIALIZE_HEADER_20160613
#define TCPIP_ROTO_SERIALIZE_HEADER_20160613

#include <string>
#include <vector>
#include <cstdint>
#include <cstring>
#include <cstdio>

#include "toolkit.h"

#if USE_SAFE_VECTOR_SIZE
#define MAXIMUM_VECTOR_ITEM_COUNT   (50000)  // vector
#define MAXIMUM_STRING_CHARACTER_COUNT  (100000) // string

#if !defined SAFE_VECTOR_SIZE_CHECKING
#define SAFE_VECTOR_SIZE_CHECKING(size)	(size < MAXIMUM_VECTOR_ITEM_COUNT)
#endif // !SAFE_SIZE_CHECKING

#if !defined SAFE_STRING_SIZE_CHECKING
#define SAFE_STRING_SIZE_CHECKING(size) (size < MAXIMUM_STRING_CHARACTER_COUNT)
#endif // !SAFE_STRING_SIZE_CHECKING

#else

#if !defined SAFE_VECTOR_SIZE_CHECKING
#define SAFE_VECTOR_SIZE_CHECKING(size)	(1)
#endif // !SAFE_SIZE_CHECKING

#if !defined SAFE_STRING_SIZE_CHECKING
#define SAFE_STRING_SIZE_CHECKING(size)	(1)
#endif // !SAFE_STRING_SIZE_CHECKING

#endif // USE_SAFE_VECTOR_SIZE



namespace nsp {
    namespace proto {

        struct proto_interface {
            virtual const int length() const = 0;
            virtual unsigned char *serialize(unsigned char *bytes) const = 0;
            virtual const unsigned char *build(const unsigned char *bytes, int &cb) = 0;

            int assemble(const std::string &bytes) {
                int cb = (int) bytes.size();
                return ( (nullptr != build((unsigned char *) bytes.data(), cb)) ? (0) : (-1));
            }
        };

        template<class T>
        struct proto_crt_t : public proto_interface {

            proto_crt_t() {
                memset(&value_, 0, length());
            }

            proto_crt_t(const T &ref) {
                value_ = ref;
            }

            proto_crt_t(const T &&ref) {
                value_ = ref;
            }

            static const int type_length() {
                return sizeof ( T);
            }

            virtual const int length() const override {
                return sizeof ( T);
            }

            virtual unsigned char *serialize(unsigned char *byte_stream) const override {
                if (!byte_stream) return nullptr;
                memcpy(byte_stream, &value_, length());
                return ( byte_stream + length());
            }

            virtual const unsigned char *build(const unsigned char *byte_stream, int &cb) override {
                if (cb < length() || !byte_stream) return nullptr;
                value_ = *((T *) byte_stream);
                cb -= length();
                return ( byte_stream + length());
            }

            operator T() {
                return value_;
            }

            operator const T() const {
                return value_;
            }

            T *operator&() {
                return &value_;
            }

            const T *operator&() const {
                return &value_;
            }

            proto_crt_t<T> &operator=(const T &c) {
                value_ = static_cast<T> (c);
                return *this;
            }
            T value_;
        };

        typedef proto_crt_t<int8_t> proto_int8_t;
        typedef proto_crt_t<uint8_t> proto_uint8_t;
        typedef proto_crt_t<int16_t> proto_int16_t;
        typedef proto_crt_t<uint16_t> proto_uint16_t;
        typedef proto_crt_t<int32_t> proto_int32_t;
        typedef proto_crt_t<uint32_t> proto_uint32_t;
        typedef proto_crt_t<int64_t> proto_int64_t;
        typedef proto_crt_t<uint64_t> proto_uint64_t;
        typedef proto_crt_t<float> proto_float32_t;
        typedef proto_crt_t<double> proto_float64_t;

        template<class T, int ENABLE_BIG_ENDIAN = 0 >
        struct proto_vector_t : public std::vector<T>, public proto_interface {

            proto_vector_t() : std::vector<T>() {
            }

            virtual const int length() const override {
                int sum = 0;
                sum += proto_crt_t<uint32_t>().length();
                for (const T &iter : * this) sum += iter.length();
                return sum;
            }

            virtual unsigned char *serialize(unsigned char *byte_stream) const override {
                unsigned char *stream_pos = byte_stream;
                proto_crt_t<uint32_t> element_count((uint32_t)this->size());
                if (ENABLE_BIG_ENDIAN) element_count = toolkit::change_byte_order(element_count.value_);
                stream_pos = element_count.serialize(stream_pos);
                for (const T &iter : * this) {
                    stream_pos = iter.serialize(stream_pos);
                    if (!stream_pos) return nullptr;
                }
                return stream_pos;
            }

            virtual const unsigned char *build(const unsigned char *byte_stream, int &cb) override {
                const unsigned char *stream_pos = byte_stream;
                proto_crt_t<uint32_t> element_count;
                stream_pos = element_count.build(stream_pos, cb);
                if (ENABLE_BIG_ENDIAN) element_count = toolkit::change_byte_order(element_count.value_);
				if ( !stream_pos ) return nullptr;
				if ( !SAFE_VECTOR_SIZE_CHECKING( element_count ) ) return nullptr;
                for (uint32_t i = 0; i < element_count; i++) {
                    T item;
                    stream_pos = item.build(stream_pos, cb);
                    if (!stream_pos) return nullptr;
                    this->push_back(std::move(item));
                }
                return stream_pos;
            }
        };

        template<class T, int ENABLE_BIG_ENDIAN = 0 >
        struct proto_string_t : public std::basic_string<T>, public proto_interface {

            proto_string_t() : std::basic_string<T>() {
            }

            proto_string_t(const T *str) : std::basic_string<T>(str) {
            }

            proto_string_t(const std::basic_string<T> &stdstr) : std::basic_string<T>(stdstr) {
            }

            virtual const int length() const override {
                int cb = 0;
                cb += proto_crt_t<uint32_t>().length();
                cb += (int) (std::basic_string<T>::size() * sizeof ( T));
                return cb;
            }

            virtual unsigned char *serialize(unsigned char *byte) const override {
                unsigned char *stream_pos = byte;
                proto_crt_t<uint32_t> element_count((uint32_t) std::basic_string<T>::size());
                if (ENABLE_BIG_ENDIAN) element_count = toolkit::change_byte_order(element_count.value_);
                stream_pos = element_count.serialize(stream_pos);
                for (const T &iter : * this) {
                    stream_pos = proto_crt_t<T>(iter).serialize(stream_pos);
                    if (!stream_pos) return nullptr;
                }
                return stream_pos;
            }

            virtual const unsigned char *build(const unsigned char *byte_stream, int &cb) override {
                const unsigned char *stream_pos = byte_stream;
                proto_crt_t<uint32_t> element_count;
                stream_pos = element_count.build(stream_pos, cb);
                if (ENABLE_BIG_ENDIAN) element_count = toolkit::change_byte_order(element_count.value_);
                if ( !stream_pos ) return nullptr;
				if ( !SAFE_STRING_SIZE_CHECKING( element_count ) ) return nullptr;
                int acquire_cb = sizeof ( T) * element_count;
                if (cb < acquire_cb) return nullptr;
                try {
                    this->assign((const T *) stream_pos, element_count);
                } catch (...) {
                    return nullptr;
                }
                cb -= acquire_cb;
                return ( stream_pos + acquire_cb);
            }

            operator const char *() const {
                return this->c_str();
            }

            proto_string_t<T> &operator=(const T *ptr) {
                std::basic_string<T>::operator=(ptr);
                return *this;
            }

            proto_string_t<T> &operator=(const std::basic_string<T> &stdstr) {
                std::basic_string<T>::operator=(stdstr);
                return *this;
            }
        };

#if 0
        template<class T, uint32_t N, template <class> class proto_container = proto_crt_t>
        struct proto_array_t : public proto_interface {

            proto_array_t() {
            }

            virtual const int length() const override {
                int sum = 0;
                for (uint32_t i = 0; i < N; i++) sum += ay_[i].length();
                return sum;
            }

            virtual unsigned char *serialize(unsigned char *byte_stream) const override {
                unsigned char *stream_pos = byte_stream;
                for (uint32_t i = 0; (i < N && stream_pos); i++) stream_pos = ay_[i].serialize(stream_pos);
                return stream_pos;
            }

            virtual const unsigned char *build(const unsigned char *byte_stream, int &cb) override {
                const unsigned char *stream_pos = byte_stream;
                for (uint32_t i = 0; (i < N && stream_pos); i++) stream_pos = ay_[i].build(stream_pos, cb);
                return stream_pos;
            }

            T &operator[](const uint32_t index) {
                if (index >= N) throw std::out_of_range("array index out of rang");
                return ay_[index];
            }

            const T &operator[](const uint32_t index) const {
                if (index >= N) throw std::out_of_range("array index out of rang");
                return ay_[index];
            }

            size_t countof() const {
                return N;
            }

            operator T *() {
                return &ay_[0];
            }

            operator const T *() const {
                return &ay_[0];
            }

            T *operator&() {
                return &ay_[0];
            }

            const T *operator&() const {
                return &ay_[0];
            }

            proto_container<T> ay_[N];
        };

#endif

    } // namespace proto
} // namespace nsp

#endif