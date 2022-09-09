
#include <math.h>






static const double fpr_q = { 12289.0 };
static const double fpr_inverse_of_q = { 1.0 / 12289.0 };
static const double fpr_inv_2sqrsigma0 = { .150865048875372721532312163019 };
static const double fpr_inv_sigma[] = {
	0.0,
	0.0069054793295940891952143765991630516,
	0.0068102267767177975961393730687908629,
	0.0067188101910722710707826117910434131,
	0.0065883354370073665545865037227681924,
    0.0064651781207602900738053897763485516,
	0.0063486788828078995327741182928037856,
	0.0062382586529084374473367528433697537,
	0.0061334065020930261548984001431770281,
	0.0060336696681577241031668062510953022,
	0.0059386453095331159950250124336477482
};
static const double fpr_sigma_min[] = {
	 0.0 ,
	 1.1165085072329102588881898380334015 ,
	 1.1321247692325272405718031785357108 ,
	 1.1475285353733668684571123112513188 ,
	 1.1702540788534828939713084716509250 ,
	 1.1925466358390344011122170489094133 ,
	 1.2144300507766139921088487776957699 ,
	 1.2359260567719808790104525941706723 ,
	 1.2570545284063214162779743112075080 ,
	 1.2778336969128335860256340575729042 ,
	 1.2982803343442918539708792538826807
};
static const double fpr_log2 = { 0.69314718055994530941723212146 };
static const double fpr_inv_log2 = { 1.4426950408889634073599246810 };
static const double fpr_bnorm_max = { 16822.4121 };
static const double fpr_zero = { 0.0 };
static const double fpr_one = { 1.0 };
static const double fpr_two = { 2.0 };
static const double fpr_onehalf = { 0.5 };
static const double fpr_invsqrt2 = { 0.707106781186547524400844362105 };
static const double fpr_invsqrt8 = { 0.353553390593273762200422181052 };
static const double fpr_ptwo31 = { 2147483648.0 };
static const double fpr_ptwo31m1 = { 2147483647.0 };
static const double fpr_mtwo31m1 = { -2147483647.0 };
static const double fpr_ptwo63m1 = { 9223372036854775807.0 };
static const double fpr_mtwo63m1 = { -9223372036854775807.0 };
static const double fpr_ptwo63 = { 9223372036854775808.0 };

static inline int64_t
fpr_rint(double x)
{
	int64_t sx, tx, rp, rn, m;
	uint32_t ub;

	sx = (int64_t)(x - 1.0);
	tx = (int64_t)x;
	rp = (int64_t)(x + 4503599627370496.0) - 4503599627370496;
	rn = (int64_t)(x - 4503599627370496.0) + 4503599627370496;


	m = sx >> 63;
	rn &= m;
	rp &= ~m;


	ub = (uint32_t)((uint64_t)tx >> 52);
	m = -(int64_t)((((ub + 1) & 0xFFF) - 2) >> 31);
	rp &= m;
	rn &= m;
	tx &= ~m;

	return tx | rn | rp;
}

static inline int64_t
fpr_floor(double x)
{
	int64_t r;
	r = (int64_t)x;
	return r - (x < (double)r);
}

static inline int64_t
fpr_trunc(double x)
{
	return (int64_t)x;
}

static inline double
fpr_add(double x, double y)
{
	return x + y;
}

static inline double
fpr_sub(double x, double y)
{
	return x - y;
}

static inline double
fpr_neg(double x)
{
	return -x;
}

static inline double
fpr_half(double x)
{
	return x * 0.5;
}

static inline double
fpr_double(double x)
{
	return x + x;
}

static inline double
fpr_mul(double x, double y)
{
	return x * y;
}

static inline double
fpr_sqr(double x)
{
	return x * x;
}

static inline double
fpr_inv(double x)
{
	return 1.0 / x;
}

static inline double
fpr_div(double x, double y)
{
	return x / y;
}



static inline double
fpr_sqrt(double x)
{
	return sqrt(x);

}

static inline int
fpr_lt(double x, double y)
{
	return x < y;
}


static inline uint64_t
fpr_expm_p63(double x, double ccs)
{
	/*
	 * Polynomial approximation of exp(-x) is taken from FACCT:
	 *   https://eprint.iacr.org/2018/1234
	 * Specifically, values are extracted from the implementation
	 * referenced from the FACCT article, and available at:
	 *   https://github.com/raykzhao/gaussian
	 * Tests over more than 24 billions of random inputs in the
	 * 0..log(2) range have never shown a deviation larger than
	 * 2^(-50) from the true mathematical value.
	 */



	/*
	 * Normal implementation uses Horner's method, which minimizes
	 * the number of operations.
	 */

	double d, y;

	d = x;
	y = 0.000000002073772366009083061987;
	y = 0.000000025299506379442070029551 - y * d;
	y = 0.000000275607356160477811864927 - y * d;
	y = 0.000002755586350219122514855659 - y * d;
	y = 0.000024801566833585381209939524 - y * d;
	y = 0.000198412739277311890541063977 - y * d;
	y = 0.001388888894063186997887560103 - y * d;
	y = 0.008333333327800835146903501993 - y * d;
	y = 0.041666666666110491190622155955 - y * d;
	y = 0.166666666666984014666397229121 - y * d;
	y = 0.500000000000019206858326015208 - y * d;
	y = 0.999999999999994892974086724280 - y * d;
	y = 1.000000000000000000000000000000 - y * d;
	y *= ccs;
	return (uint64_t)(y * fpr_ptwo63);


}


extern const double fpr_gm_tab[];


extern const double fpr_p2_tab[];

/* ====================================================================== */

