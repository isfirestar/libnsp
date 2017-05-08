#if !defined TOOLKIT_HEADER_02160613
#define TOOLKIT_HEADER_02160613

#include "icom/compiler.h"

#include <vector>

namespace nsp {
	namespace toolkit {

		// �� @n ��������һ��2����������
		// ������� @n ���� 2 ���������ݣ�ֻ���ҵ�����ߵ���Чλ1���ڵ�λ�ã���1��ʼ������pos��Ȼ��1 << pos���ɽ�k����ȡ��Ϊ2����������
		template<class T>
		inline T roundup_powerof_2( T n ) {
			if ( is_powerof_2( n ) ) {
				return n;
			}
			// ���ٱ�֤ 2 �� 1 ����
			if ( 0 == n ) {
				return 2;
			}
			uint32_t position = 0;
			for ( int i = n; i != 0; i >>= 1 ) {
				position++;
			}
			return static_cast< T >( ( T ) 1 << position );
		}
        
        template<class T>
        T *posix_strcpy(T *target, std::size_t cch, const T *src);
        template<class T>
        T *posix_strdup(const T *src);
        template<class T>
        T *posix_strncpy(T *target, std::size_t cch, const T *src, std::size_t cnt);
        template<class T>
        T *posix_strcat(T *target, std::size_t cch, const T *src);
        template<class T>
        T *posix_strrev(T *src);
        template<class T>
        int posix_vsnprintf(T * const target, std::size_t cch, const T *format, va_list ap);
        template<class T>
        int posix_vsprintf(T * const target, std::size_t cch, const T *format, va_list ap);
        template<class T>
        int posix_strcasecmp(const T *s1, const T *s2);
        template<class T>
        T *posix_strtok(T *s, const T *, T **save);

        int is_digit_str(const std::string &str);
        int is_effective_ipv4str(const std::string &ipstr);
        int is_valid_portstr(const std::string &portstr, uint16_t &port);
        uint32_t ipv4_touint(const char *ipv4str, int method);
        char *ipv4_tostring(uint32_t ipv4Integer, char * ipv4TextString, uint32_t lengthOfTextCch);

        template<class T>
        std::basic_string<T> to_string(const double d);
        template<class T>
        std::basic_string<T> to_string(const uint16_t d);
        template<class T>
        std::basic_string<T> to_string(const int16_t d);
        template<class T>
        std::basic_string<T> to_string(const uint32_t d);
        template<class T>
        std::basic_string<T> to_string(const int32_t d);
        template<class T>
        std::basic_string<T> to_string(const uint64_t d);
        template<class T>
        std::basic_string<T> to_string(const int64_t d);

        template<class T>
        std::basic_string<T> strformat(int cch, const T *format, ...);

        // �ֽ���ת������

        template<class T>
        T change_byte_order(const T &t) {
            T dst = 0;
            for (int i = 0; i < sizeof ( T); i++) {
                // dst = (dst | (t >> (i * BITS_P_BYTE)) & 0xFF) << ((i < (sizeof ( T) - 1) ? BITS_P_BYTE : 0));
                dst |= ((t >> (i * BITS_P_BYTE)) & 0xFF);
                dst <<= ((i) < (sizeof ( T) - 1) ? BITS_P_BYTE : (0));
            }
            return dst;
        }
        uint32_t htonl(const uint32_t l);
        unsigned short htons(const uint16_t s);
        uint32_t ntohl(const uint32_t l);
        unsigned short ntohs(const uint16_t s);

        template<class T>
        T *trim_space(const T *inputString, T * outputString, std::size_t cch);

        // ���Լ��, greatest common divisor
        int gcd(int a, int b);
        // ��С������, Least common multiple
        double lcm(int a, int b);
        // �Ƕ�ת����
        double deg2rad(double angle);
        // ����ת�Ƕ�
        double rad2deg(double radian);
        // ������ɢ����Ҷ
        int dispersed_fourier_transform(int dir, int m, double *x1, double *y1);

        // ���ָ����и��Ӵ�
        template<class T>
        std::size_t slicing_symbol_string(const std::basic_string<T> &source, const T symbol, std::vector<std::basic_string<T>> &vct_substr);

        // ȥ�����ҿո�
        template<class T>
        int trim(const std::basic_string<T> &src, std::basic_string<T> &dst);
        template<class T>
        void trim(std::basic_string<T> &str);

        // ����ԭ����ȡ�����ҿո�󷵻��´�
        template<class T>
        std::basic_string<T> trim_copy(const std::basic_string<T> &src);

        // ȡ����� [range_min, range_max)
        // �������Ϊ (0,0), ��ΧΪĬ�ϵ�[0, 32768)
        // ��� range_max < range_min, ��Ĭ�ϴ��� ��Χ����Ч
        // ��� range_max == range_min, �ҷ�0�� �򷵻ظ�ֵ
        int random(const int range_min = 0, const int range_max = 0);

        // �ԳƼӽ�������
        int encrypt(const unsigned char* origin, int oricb, const unsigned char* key, int keycb, unsigned char** out, int* outcb);
        template<class T>
        int encrypt(const std::basic_string<T> &origin, const std::basic_string<T> &key, std::string &out);
        template<class T>
        int encrypt(const std::vector<T> &origin, const std::vector<T> &key, std::string &out);
        int encrypt(const char *origin, const char *key, std::string &out);

        int decrypt(const unsigned char* crypt, int cryptcb, const unsigned char* key, int keycb, unsigned char** out, int* outcb);
        template<class T>
        int decrypt(const std::string &crypt, const std::basic_string<T> &key, std::basic_string<T> &out);
        template<class T>
        int decrypt(const std::string &crypt, const std::vector<T> &key, std::vector<T> &out);
        int decrypt(const std::string &crypt, const char *key, std::string &out);

        // HEX�ַ���ת����, "1234ABCD" ת��Ϊ 0x1234ABCD�� ע��ߵ�λ�ṹ
        template<class T>
        int strtohex(const std::basic_string<char> &strhex, T &int_hex) {
            // �����ʼֵ
            int_hex = 0;

            // Ȩֵ
            int access = strhex.length() - 1;

            // ������������Χ
            if (strhex.length() > (sizeof ( T) * 2)) {
                return -1;
            }

            for (std::size_t i = 0; i < strhex.length(); i++) {
                char c = strhex.at(i);
                if (c >= 0x30 && c <= 0x39) {
                    int_hex += (T) ((c - 0x30) * ((access > 0) ? pow((double) 16.0, access) : 1));
                } else {
                    if (c >= 0x41 && c <= 0x46) {
                        int_hex += (T) ((c - 0x37) * ((access > 0) ? pow((double) 16.0, access) : 1));
                    } else if (c >= 0x61 && c <= 0x66) {
                        int_hex += (T) ((c - 0x57) * ((access > 0) ? pow((double) 16.0, access) : 1));
                    } else {
                        return -1;
                    }
                }
                access--;
            }

            return 0;
        }

        // ��ȡ CRC ��
        uint32_t crc32(uint32_t crc, const unsigned char *block, uint32_t cb);

        // ��ȡ MD5 ��
        template<class T>
        void md5(const T *input, int inputlen, unsigned char digest[16]);

        // base64 �����
        int base64_encode(const char *input, int cb, std::string &output);
        int base64_encode(const std::string &input, std::string &output);
		int base64_decode( const std::string &input, std::string &output );

		// ����ת��
		unsigned short float2fixed( float fFloat );
		float fixed2float( unsigned short uFixed );

    } // toolkit
} // nsp

#endif