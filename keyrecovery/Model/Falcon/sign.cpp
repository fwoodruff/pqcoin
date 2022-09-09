/*
 * Falcon signature generation.
 *
 * ==========================(LICENSE BEGIN)============================
 *
 * Copyright (c) 2017-2019  Falcon Project
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * ===========================(LICENSE END)=============================
 *
 * @author   Thomas Pornin <thomas.pornin@nccgroup.com>
 */

#include "inner.h"

/* =================================================================== */

/*
 * Compute degree N from logarithm 'logn'.
 */
#define MKN(logn)   ((size_t)1 << (logn))

/* =================================================================== */
/*
 * Binary case:
 *   N = 2^logn
 *   phi = X^N+1
 */

/*
 * Get the size of the LDL tree for an input with polynomials of size
 * 2^logn. The size is expressed in the number of elements.
 */
static inline unsigned
ffLDL_treesize(unsigned logn)
{
	/*
	 * For logn = 0 (polynomials are constant), the "tree" is a
	 * single element. Otherwise, the tree node has size 2^logn, and
	 * has two child trees for size logn-1 each. Thus, treesize s()
	 * must fulfill these two relations:
	 *
	 *   s(0) = 1
	 *   s(logn) = (2^logn) + 2*s(logn-1)
	 */
	return (logn + 1) << logn;
}

/*
 * Inner function for ffLDL_fft(). It expects the matrix to be both
 * auto-adjoint and quasicyclic; also, it uses the source operands
 * as modifiable temporaries.
 *
 * tmp[] must have room for at least one polynomial.
 */
static void
ffLDL_fft_inner(double * tree, double * g0, double * g1, unsigned logn, double * tmp)
{
	size_t n, hn;

	n = MKN(logn);
	if (n == 1) {
		tree[0] = g0[0];
		return;
	}
	hn = n >> 1;

	/*
	 * The LDL decomposition yields L (which is written in the tree)
	 * and the diagonal of D. Since d00 = g0, we just write d11
	 * into tmp.
	 */
	poly_LDLmv_fft(tmp, tree, g0, g1, g0, logn);

	/*
	 * Split d00 (currently in g0) and d11 (currently in tmp). We
	 * reuse g0 and g1 as temporary storage spaces:
	 *   d00 splits into g1, g1+hn
	 *   d11 splits into g0, g0+hn
	 */
	poly_split_fft(g1, g1 + hn, g0, logn);
	poly_split_fft(g0, g0 + hn, tmp, logn);

	/*
	 * Each split result is the first row of a new auto-adjoint
	 * quasicyclic matrix for the next recursive step.
	 */
	ffLDL_fft_inner(tree + n,
		g1, g1 + hn, logn - 1, tmp);
	ffLDL_fft_inner(tree + n + ffLDL_treesize(logn - 1),
		g0, g0 + hn, logn - 1, tmp);
}

/*
 * Compute the ffLDL tree of an auto-adjoint matrix G. The matrix
 * is provided as three polynomials (FFT representation).
 *
 * The "tree" array is filled with the computed tree, of size
 * (logn+1)*(2^logn) elements (see ffLDL_treesize()).
 *
 * Input arrays MUST NOT overlap, except possibly the three unmodified
 * arrays g00, g01 and g11. tmp[] should have room for at least three
 * polynomials of 2^logn elements each.
 */
static void
ffLDL_fft(double *tree, const double *g00,
	const double *g01, const double * g11,
	unsigned logn, double * tmp)
{
	size_t n, hn;
	double *d00, *d11;

	n = MKN(logn);
	if (n == 1) {
		tree[0] = g00[0];
		return;
	}
	hn = n >> 1;
	d00 = tmp;
	d11 = tmp + n;
	tmp += n << 1;

	memcpy(d00, g00, n * sizeof *g00);
	poly_LDLmv_fft(d11, tree, g00, g01, g11, logn);

	poly_split_fft(tmp, tmp + hn, d00, logn);
	poly_split_fft(d00, d00 + hn, d11, logn);
	memcpy(d11, tmp, n * sizeof *tmp);
	ffLDL_fft_inner(tree + n,
		d11, d11 + hn, logn - 1, tmp);
	ffLDL_fft_inner(tree + n + ffLDL_treesize(logn - 1),
		d00, d00 + hn, logn - 1, tmp);
}

/*
 * Normalize an ffLDL tree: each leaf of value x is replaced with
 * sigma / sqrt(x).
 */
static void
ffLDL_binary_normalize(double *tree, unsigned orig_logn, unsigned logn)
{
	/*
	 * TODO: make an iterative version.
	 */
	size_t n;

	n = MKN(logn);
	if (n == 1) {
		/*
		 * We actually store in the tree leaf the inverse of
		 * the value mandated by the specification: this
		 * saves a division both here and in the sampler.
		 */
		tree[0] = fpr_mul(fpr_sqrt(tree[0]), fpr_inv_sigma[orig_logn]);
	} else {
		ffLDL_binary_normalize(tree + n, orig_logn, logn - 1);
		ffLDL_binary_normalize(tree + n + ffLDL_treesize(logn - 1),
			orig_logn, logn - 1);
	}
}

/* =================================================================== */

/*
 * Convert an integer polynomial (with small values) into the
 * representation with complex numbers.
 */
static void
smallints_to_fpr(double *r, const int8_t *t, unsigned logn)
{
	size_t n, u;

	n = MKN(logn);
	for (u = 0; u < n; u ++) {
		r[u] = t[u];
	}
}

/*
 * The expanded private key contains:
 *  - The B0 matrix (four elements)
 *  - The ffLDL tree
 */

static inline size_t
skoff_b00(unsigned logn)
{
	(void)logn;
	return 0;
}

static inline size_t
skoff_b01(unsigned logn)
{
	return MKN(logn);
}

static inline size_t
skoff_b10(unsigned logn)
{
	return 2 * MKN(logn);
}

static inline size_t
skoff_b11(unsigned logn)
{
	return 3 * MKN(logn);
}

static inline size_t
skoff_tree(unsigned logn)
{
	return 4 * MKN(logn);
}

/* see inner.h */
void
expand_privkey(double * expanded_key,
	const int8_t *f, const int8_t *g,
	const int8_t *F, const int8_t *G,
	unsigned logn, uint8_t * tmp)
{
	size_t n;
	double *rf, *rg, *rF, *rG;
	double *b00, *b01, *b10, *b11;
	double *g00, *g01, *g11, *gxx;
	double *tree;

	n = MKN(logn);
	b00 = expanded_key + skoff_b00(logn);
	b01 = expanded_key + skoff_b01(logn);
	b10 = expanded_key + skoff_b10(logn);
	b11 = expanded_key + skoff_b11(logn);
	tree = expanded_key + skoff_tree(logn);

	/*
	 * We load the private key elements directly into the B0 matrix,
	 * since B0 = [[g, -f], [G, -F]].
	 */
	rf = b01;
	rg = b00;
	rF = b11;
	rG = b10;

	smallints_to_fpr(rf, f, logn);
	smallints_to_fpr(rg, g, logn);
	smallints_to_fpr(rF, F, logn);
	smallints_to_fpr(rG, G, logn);

	/*
	 * Compute the FFT for the key elements, and negate f and F.
	 */
	FFT(rf, logn);
	FFT(rg, logn);
	FFT(rF, logn);
	FFT(rG, logn);
	poly_neg(rf, logn);
	poly_neg(rF, logn);

	/*
	 * The Gram matrix is G = B·B*. Formulas are:
	 *   g00 = b00*adj(b00) + b01*adj(b01)
	 *   g01 = b00*adj(b10) + b01*adj(b11)
	 *   g10 = b10*adj(b00) + b11*adj(b01)
	 *   g11 = b10*adj(b10) + b11*adj(b11)
	 *
	 * For historical reasons, this implementation uses
	 * g00, g01 and g11 (upper triangle).
	 */
	g00 = (double *)tmp;
	g01 = g00 + n;
	g11 = g01 + n;
	gxx = g11 + n;

	memcpy(g00, b00, n * sizeof *b00);
	poly_mulselfadj_fft(g00, logn);
	memcpy(gxx, b01, n * sizeof *b01);
	poly_mulselfadj_fft(gxx, logn);
	poly_add(g00, gxx, logn);

	memcpy(g01, b00, n * sizeof *b00);
	poly_muladj_fft(g01, b10, logn);
	memcpy(gxx, b01, n * sizeof *b01);
	poly_muladj_fft(gxx, b11, logn);
	poly_add(g01, gxx, logn);

	memcpy(g11, b10, n * sizeof *b10);
	poly_mulselfadj_fft(g11, logn);
	memcpy(gxx, b11, n * sizeof *b11);
	poly_mulselfadj_fft(gxx, logn);
	poly_add(g11, gxx, logn);


	ffLDL_fft(tree, g00, g01, g11, logn, gxx);


	ffLDL_binary_normalize(tree, logn, logn);
}

typedef int (*samplerZ)(void *ctx, double mu, double sigma);



static void
ffSampling_fft_dyntree(samplerZ samp, void *samp_ctx,
	double * t0, double * t1,
	double * g00, double * g01, double * g11,
	unsigned orig_logn, unsigned logn, double * tmp)
{
	size_t n, hn;
	double *z0, *z1;

	if (logn == 0) {
		double leaf;

		leaf = g00[0];
		leaf = fpr_mul(fpr_sqrt(leaf), fpr_inv_sigma[orig_logn]);
		t0[0] = samp(samp_ctx, t0[0], leaf);
		t1[0] = samp(samp_ctx, t1[0], leaf);
		return;
	}

	n = (size_t)1 << logn;
	hn = n >> 1;

	poly_LDL_fft(g00, g01, g11, logn);

	
	poly_split_fft(tmp, tmp + hn, g00, logn);
	memcpy(g00, tmp, n * sizeof *tmp);
	poly_split_fft(tmp, tmp + hn, g11, logn);
	memcpy(g11, tmp, n * sizeof *tmp);
	memcpy(tmp, g01, n * sizeof *g01);
	memcpy(g01, g00, hn * sizeof *g00);
	memcpy(g01 + hn, g11, hn * sizeof *g00);

	z1 = tmp + n;
	poly_split_fft(z1, z1 + hn, t1, logn);
	ffSampling_fft_dyntree(samp, samp_ctx, z1, z1 + hn,
		g11, g11 + hn, g01 + hn, orig_logn, logn - 1, z1 + n);
	poly_merge_fft(tmp + (n << 1), z1, z1 + hn, logn);

	memcpy(z1, t1, n * sizeof *t1);
	poly_sub(z1, tmp + (n << 1), logn);
	memcpy(t1, tmp + (n << 1), n * sizeof *tmp);
	poly_mul_fft(tmp, z1, logn);
	poly_add(t0, tmp, logn);

	/*
	 * Second recursive invocation, on the split tb0 (currently in t0)
	 * and the left sub-tree.
	 */
	z0 = tmp;
	poly_split_fft(z0, z0 + hn, t0, logn);
	ffSampling_fft_dyntree(samp, samp_ctx, z0, z0 + hn,
		g00, g00 + hn, g01, orig_logn, logn - 1, z0 + n);
	poly_merge_fft(t0, z0, z0 + hn, logn);
}


static void
ffSampling_fft(samplerZ samp, void *samp_ctx,
	double * z0, double * z1,
	const double * tree,
	const double * t0, const double * t1, unsigned logn,
	double * tmp)
{
	size_t n, hn;
	const double *tree0, *tree1;

	/*
	 * When logn == 2, we inline the last two recursion levels.
	 */
	if (logn == 2) {

		double x0, x1, y0, y1, w0, w1, w2, w3, sigma;
		double a_re, a_im, b_re, b_im, c_re, c_im;

		tree0 = tree + 4;
		tree1 = tree + 8;

		/*
		 * We split t1 into w*, then do the recursive invocation,
		 * with output in w*. We finally merge back into z1.
		 */
		a_re = t1[0];
		a_im = t1[2];
		b_re = t1[1];
		b_im = t1[3];
		c_re = fpr_add(a_re, b_re);
		c_im = fpr_add(a_im, b_im);
		w0 = fpr_half(c_re);
		w1 = fpr_half(c_im);
		c_re = fpr_sub(a_re, b_re);
		c_im = fpr_sub(a_im, b_im);
		w2 = fpr_mul(fpr_add(c_re, c_im), fpr_invsqrt8);
		w3 = fpr_mul(fpr_sub(c_im, c_re), fpr_invsqrt8);

		x0 = w2;
		x1 = w3;
		sigma = tree1[3];
		w2 = samp(samp_ctx, x0, sigma);
		w3 = samp(samp_ctx, x1, sigma);
		a_re = fpr_sub(x0, w2);
		a_im = fpr_sub(x1, w3);
		b_re = tree1[0];
		b_im = tree1[1];
		c_re = fpr_sub(fpr_mul(a_re, b_re), fpr_mul(a_im, b_im));
		c_im = fpr_add(fpr_mul(a_re, b_im), fpr_mul(a_im, b_re));
		x0 = fpr_add(c_re, w0);
		x1 = fpr_add(c_im, w1);
		sigma = tree1[2];
		w0 = samp(samp_ctx, x0, sigma);
		w1 = samp(samp_ctx, x1, sigma);

		a_re = w0;
		a_im = w1;
		b_re = w2;
		b_im = w3;
		c_re = fpr_mul(fpr_sub(b_re, b_im), fpr_invsqrt2);
		c_im = fpr_mul(fpr_add(b_re, b_im), fpr_invsqrt2);
		z1[0] = w0 = fpr_add(a_re, c_re);
		z1[2] = w2 = fpr_add(a_im, c_im);
		z1[1] = w1 = fpr_sub(a_re, c_re);
		z1[3] = w3 = fpr_sub(a_im, c_im);

		/*
		 * Compute tb0 = t0 + (t1 - z1) * L. Value tb0 ends up in w*.
		 */
		w0 = fpr_sub(t1[0], w0);
		w1 = fpr_sub(t1[1], w1);
		w2 = fpr_sub(t1[2], w2);
		w3 = fpr_sub(t1[3], w3);

		a_re = w0;
		a_im = w2;
		b_re = tree[0];
		b_im = tree[2];
		w0 = fpr_sub(fpr_mul(a_re, b_re), fpr_mul(a_im, b_im));
		w2 = fpr_add(fpr_mul(a_re, b_im), fpr_mul(a_im, b_re));
		a_re = w1;
		a_im = w3;
		b_re = tree[1];
		b_im = tree[3];
		w1 = fpr_sub(fpr_mul(a_re, b_re), fpr_mul(a_im, b_im));
		w3 = fpr_add(fpr_mul(a_re, b_im), fpr_mul(a_im, b_re));

		w0 = fpr_add(w0, t0[0]);
		w1 = fpr_add(w1, t0[1]);
		w2 = fpr_add(w2, t0[2]);
		w3 = fpr_add(w3, t0[3]);

		/*
		 * Second recursive invocation.
		 */
		a_re = w0;
		a_im = w2;
		b_re = w1;
		b_im = w3;
		c_re = fpr_add(a_re, b_re);
		c_im = fpr_add(a_im, b_im);
		w0 = fpr_half(c_re);
		w1 = fpr_half(c_im);
		c_re = fpr_sub(a_re, b_re);
		c_im = fpr_sub(a_im, b_im);
		w2 = fpr_mul(fpr_add(c_re, c_im), fpr_invsqrt8);
		w3 = fpr_mul(fpr_sub(c_im, c_re), fpr_invsqrt8);

		x0 = w2;
		x1 = w3;
		sigma = tree0[3];
		w2 = y0 = samp(samp_ctx, x0, sigma);
		w3 = y1 = samp(samp_ctx, x1, sigma);
		a_re = fpr_sub(x0, y0);
		a_im = fpr_sub(x1, y1);
		b_re = tree0[0];
		b_im = tree0[1];
		c_re = fpr_sub(fpr_mul(a_re, b_re), fpr_mul(a_im, b_im));
		c_im = fpr_add(fpr_mul(a_re, b_im), fpr_mul(a_im, b_re));
		x0 = fpr_add(c_re, w0);
		x1 = fpr_add(c_im, w1);
		sigma = tree0[2];
		w0 = samp(samp_ctx, x0, sigma);
		w1 = samp(samp_ctx, x1, sigma);

		a_re = w0;
		a_im = w1;
		b_re = w2;
		b_im = w3;
		c_re = fpr_mul(fpr_sub(b_re, b_im), fpr_invsqrt2);
		c_im = fpr_mul(fpr_add(b_re, b_im), fpr_invsqrt2);
		z0[0] = fpr_add(a_re, c_re);
		z0[2] = fpr_add(a_im, c_im);
		z0[1] = fpr_sub(a_re, c_re);
		z0[3] = fpr_sub(a_im, c_im);

		return;

	}

	if (logn == 1) {
		double x0, x1, y0, y1, sigma;
		double a_re, a_im, b_re, b_im, c_re, c_im;

		x0 = t1[0];
		x1 = t1[1];
		sigma = tree[3];
		z1[0] = y0 = samp(samp_ctx, x0, sigma);
		z1[1] = y1 = samp(samp_ctx, x1, sigma);
		a_re = fpr_sub(x0, y0);
		a_im = fpr_sub(x1, y1);
		b_re = tree[0];
		b_im = tree[1];
		c_re = fpr_sub(fpr_mul(a_re, b_re), fpr_mul(a_im, b_im));
		c_im = fpr_add(fpr_mul(a_re, b_im), fpr_mul(a_im, b_re));
		x0 = fpr_add(c_re, t0[0]);
		x1 = fpr_add(c_im, t0[1]);
		sigma = tree[2];
		z0[0] = samp(samp_ctx, x0, sigma);
		z0[1] = samp(samp_ctx, x1, sigma);

		return;
	}


	n = (size_t)1 << logn;
	hn = n >> 1;
	tree0 = tree + n;
	tree1 = tree + n + ffLDL_treesize(logn - 1);

	poly_split_fft(z1, z1 + hn, t1, logn);
	ffSampling_fft(samp, samp_ctx, tmp, tmp + hn,
		tree1, z1, z1 + hn, logn - 1, tmp + n);
	poly_merge_fft(z1, tmp, tmp + hn, logn);

	memcpy(tmp, t1, n * sizeof *t1);
	poly_sub(tmp, z1, logn);
	poly_mul_fft(tmp, tree, logn);
	poly_add(tmp, t0, logn);

	poly_split_fft(z0, z0 + hn, tmp, logn);
	ffSampling_fft(samp, samp_ctx, tmp, tmp + hn,
		tree0, z0, z0 + hn, logn - 1, tmp + n);
	poly_merge_fft(z0, tmp, tmp + hn, logn);
}

static int
do_sign_tree(samplerZ samp, void *samp_ctx, int16_t *s2,
	const double * expanded_key,
	const uint16_t *hm,
	unsigned logn, double * tmp)
{
	size_t n, u;
	double *t0, *t1, *tx, *ty;
	const double *b00, *b01, *b10, *b11, *tree;
	double ni;
	uint32_t sqn, ng;
	int16_t *s1tmp, *s2tmp;

	n = MKN(logn);
	t0 = tmp;
	t1 = t0 + n;
	b00 = expanded_key + skoff_b00(logn);
	b01 = expanded_key + skoff_b01(logn);
	b10 = expanded_key + skoff_b10(logn);
	b11 = expanded_key + skoff_b11(logn);
	tree = expanded_key + skoff_tree(logn);


	for (u = 0; u < n; u ++) {
		t0[u] = hm[u];
		/* This is implicit.
		t1[u] = fpr_zero;
		*/
	}

	FFT(t0, logn);
	ni = fpr_inverse_of_q;
	memcpy(t1, t0, n * sizeof *t0);
	poly_mul_fft(t1, b01, logn);
	poly_mulconst(t1, fpr_neg(ni), logn);
	poly_mul_fft(t0, b11, logn);
	poly_mulconst(t0, ni, logn);

	tx = t1 + n;
	ty = tx + n;

	ffSampling_fft(samp, samp_ctx, tx, ty, tree, t0, t1, logn, ty + n);


	memcpy(t0, tx, n * sizeof *tx);
	memcpy(t1, ty, n * sizeof *ty);
	poly_mul_fft(tx, b00, logn);
	poly_mul_fft(ty, b10, logn);
	poly_add(tx, ty, logn);
	memcpy(ty, t0, n * sizeof *t0);
	poly_mul_fft(ty, b01, logn);

	memcpy(t0, tx, n * sizeof *tx);
	poly_mul_fft(t1, b11, logn);
	poly_add(t1, ty, logn);

	iFFT(t0, logn);
	iFFT(t1, logn);


	s1tmp = (int16_t *)tx;
	sqn = 0;
	ng = 0;
	for (u = 0; u < n; u ++) {
		int32_t z;

		z = (int32_t)hm[u] - (int32_t)fpr_rint(t0[u]);
		sqn += (uint32_t)(z * z);
		ng |= sqn;
		s1tmp[u] = (int16_t)z;
	}
	sqn |= -(ng >> 31);


	s2tmp = (int16_t *)tmp;
	for (u = 0; u < n; u ++) {
		s2tmp[u] = (int16_t)-fpr_rint(t1[u]);
	}
	if (is_short_half(sqn, s2tmp, logn)) {
		memcpy(s2, s2tmp, n * sizeof *s2);
		memcpy(tmp, s1tmp, n * sizeof *s1tmp);
		return 1;
	}
	return 0;
}

static int
do_sign_dyn(samplerZ samp, void *samp_ctx, int16_t *s2,
	const int8_t * f, const int8_t * g,
	const int8_t * F, const int8_t * G,
	const uint16_t *hm, unsigned logn, double * tmp)
{
	size_t n, u;
	double *t0, *t1, *tx, *ty;
	double *b00, *b01, *b10, *b11, *g00, *g01, *g11;
	double ni;
	uint32_t sqn, ng;
	int16_t *s1tmp, *s2tmp;

	n = MKN(logn);


	b00 = tmp;
	b01 = b00 + n;
	b10 = b01 + n;
	b11 = b10 + n;
	smallints_to_fpr(b01, f, logn);
	smallints_to_fpr(b00, g, logn);
	smallints_to_fpr(b11, F, logn);
	smallints_to_fpr(b10, G, logn);
	FFT(b01, logn);
	FFT(b00, logn);
	FFT(b11, logn);
	FFT(b10, logn);
	poly_neg(b01, logn);
	poly_neg(b11, logn);

	t0 = b11 + n;
	t1 = t0 + n;

	memcpy(t0, b01, n * sizeof *b01);
	poly_mulselfadj_fft(t0, logn);    // t0 <- b01*adj(b01)

	memcpy(t1, b00, n * sizeof *b00);
	poly_muladj_fft(t1, b10, logn);   // t1 <- b00*adj(b10)
	poly_mulselfadj_fft(b00, logn);   // b00 <- b00*adj(b00)
	poly_add(b00, t0, logn);      // b00 <- g00
	memcpy(t0, b01, n * sizeof *b01);
	poly_muladj_fft(b01, b11, logn);  // b01 <- b01*adj(b11)
	poly_add(b01, t1, logn);      // b01 <- g01

	poly_mulselfadj_fft(b10, logn);   // b10 <- b10*adj(b10)
	memcpy(t1, b11, n * sizeof *b11);
	poly_mulselfadj_fft(t1, logn);    // t1 <- b11*adj(b11)
	poly_add(b10, t1, logn);      // b10 <- g11

	g00 = b00;
	g01 = b01;
	g11 = b10;
	b01 = t0;
	t0 = b01 + n;
	t1 = t0 + n;

	for (u = 0; u < n; u ++) {
		t0[u] = hm[u];
		/* This is implicit.
		t1[u] = fpr_zero;
		*/
	}

	FFT(t0, logn);
	ni = fpr_inverse_of_q;
	memcpy(t1, t0, n * sizeof *t0);
	poly_mul_fft(t1, b01, logn);
	poly_mulconst(t1, fpr_neg(ni), logn);
	poly_mul_fft(t0, b11, logn);
	poly_mulconst(t0, ni, logn);

	memcpy(b11, t0, n * 2 * sizeof *t0);
	t0 = g11 + n;
	t1 = t0 + n;


	ffSampling_fft_dyntree(samp, samp_ctx,
		t0, t1, g00, g01, g11, logn, logn, t1 + n);


	b00 = tmp;
	b01 = b00 + n;
	b10 = b01 + n;
	b11 = b10 + n;
	memmove(b11 + n, t0, n * 2 * sizeof *t0);
	t0 = b11 + n;
	t1 = t0 + n;
	smallints_to_fpr(b01, f, logn);
	smallints_to_fpr(b00, g, logn);
	smallints_to_fpr(b11, F, logn);
	smallints_to_fpr(b10, G, logn);
	FFT(b01, logn);
	FFT(b00, logn);
	FFT(b11, logn);
	FFT(b10, logn);
	poly_neg(b01, logn);
	poly_neg(b11, logn);
	tx = t1 + n;
	ty = tx + n;


	memcpy(tx, t0, n * sizeof *t0);
	memcpy(ty, t1, n * sizeof *t1);
	poly_mul_fft(tx, b00, logn);
	poly_mul_fft(ty, b10, logn);
	poly_add(tx, ty, logn);
	memcpy(ty, t0, n * sizeof *t0);
	poly_mul_fft(ty, b01, logn);

	memcpy(t0, tx, n * sizeof *tx);
	poly_mul_fft(t1, b11, logn);
	poly_add(t1, ty, logn);
	iFFT(t0, logn);
	iFFT(t1, logn);

	s1tmp = (int16_t *)tx;
	sqn = 0;
	ng = 0;
	for (u = 0; u < n; u ++) {
		int32_t z;

		z = (int32_t)hm[u] - (int32_t)fpr_rint(t0[u]);
		sqn += (uint32_t)(z * z);
		ng |= sqn;
		s1tmp[u] = (int16_t)z;
	}
	sqn |= -(ng >> 31);


	s2tmp = (int16_t *)tmp;
	for (u = 0; u < n; u ++) {
		s2tmp[u] = (int16_t)-fpr_rint(t1[u]);
	}
	if (is_short_half(sqn, s2tmp, logn)) {
		memcpy(s2, s2tmp, n * sizeof *s2);
		memcpy(tmp, s1tmp, n * sizeof *s1tmp);
		return 1;
	}
	return 0;
}


int
gaussian0_sampler(prng *p)
{

	static const uint32_t dist[] = {
		10745844u,  3068844u,  3741698u,
		 5559083u,  1580863u,  8248194u,
		 2260429u, 13669192u,  2736639u,
		  708981u,  4421575u, 10046180u,
		  169348u,  7122675u,  4136815u,
		   30538u, 13063405u,  7650655u,
		    4132u, 14505003u,  7826148u,
		     417u, 16768101u, 11363290u,
		      31u,  8444042u,  8086568u,
		       1u, 12844466u,   265321u,
		       0u,  1232676u, 13644283u,
		       0u,    38047u,  9111839u,
		       0u,      870u,  6138264u,
		       0u,       14u, 12545723u,
		       0u,        0u,  3104126u,
		       0u,        0u,    28824u,
		       0u,        0u,      198u,
		       0u,        0u,        1u
	};

	uint32_t v0, v1, v2, hi;
	uint64_t lo;
	size_t u;
	int z;

	/*
	 * Get a random 72-bit value, into three 24-bit limbs v0..v2.
	 */
	lo = prng_get_u64(p);
	hi = prng_get_u8(p);
	v0 = (uint32_t)lo & 0xFFFFFF;
	v1 = (uint32_t)(lo >> 24) & 0xFFFFFF;
	v2 = (uint32_t)(lo >> 48) | (hi << 16);

	/*
	 * Sampled value is z, such that v0..v2 is lower than the first
	 * z elements of the table.
	 */
	z = 0;
	for (u = 0; u < (sizeof dist) / sizeof(dist[0]); u += 3) {
		uint32_t w0, w1, w2, cc;

		w0 = dist[u + 2];
		w1 = dist[u + 1];
		w2 = dist[u + 0];
		cc = (v0 - w0) >> 31;
		cc = (v1 - w1 - cc) >> 31;
		cc = (v2 - w2 - cc) >> 31;
		z += (int)cc;
	}
	return z;

}

/*
 * Sample a bit with probability exp(-x) for some x >= 0.
 */
static int
BerExp(prng *p, double x, double ccs)
{
	int s, i;
	double r;
	uint32_t sw, w;
	uint64_t z;

	/*
	 * Reduce x modulo log(2): x = s*log(2) + r, with s an integer,
	 * and 0 <= r < log(2). Since x >= 0, we can use fpr_trunc().
	 */
	s = (int)fpr_trunc(fpr_mul(x, fpr_inv_log2));
	r = fpr_sub(x, fpr_mul(s, fpr_log2));

	/*
	 * It may happen (quite rarely) that s >= 64; if sigma = 1.2
	 * (the minimum value for sigma), r = 0 and b = 1, then we get
	 * s >= 64 if the half-Gaussian produced a z >= 13, which happens
	 * with probability about 0.000000000230383991, which is
	 * approximatively equal to 2^(-32). In any case, if s >= 64,
	 * then BerExp will be non-zero with probability less than
	 * 2^(-64), so we can simply saturate s at 63.
	 */
	sw = (uint32_t)s;
	sw ^= (sw ^ 63) & -((63 - sw) >> 31);
	s = (int)sw;

	/*
	 * Compute exp(-r); we know that 0 <= r < log(2) at this point, so
	 * we can use fpr_expm_p63(), which yields a result scaled to 2^63.
	 * We scale it up to 2^64, then right-shift it by s bits because
	 * we really want exp(-x) = 2^(-s)*exp(-r).
	 *
	 * The "-1" operation makes sure that the value fits on 64 bits
	 * (i.e. if r = 0, we may get 2^64, and we prefer 2^64-1 in that
	 * case). The bias is negligible since fpr_expm_p63() only computes
	 * with 51 bits of precision or so.
	 */
	z = ((fpr_expm_p63(r, ccs) << 1) - 1) >> s;

	/*
	 * Sample a bit with probability exp(-x). Since x = s*log(2) + r,
	 * exp(-x) = 2^-s * exp(-r), we compare lazily exp(-x) with the
	 * PRNG output to limit its consumption, the sign of the difference
	 * yields the expected result.
	 */
	i = 64;
	do {
		i -= 8;
		w = prng_get_u8(p) - ((uint32_t)(z >> i) & 0xFF);
	} while (!w && i > 0);
	return (int)(w >> 31);
}


int
sampler(void *ctx, double mu, double isigma)
{
	sampler_context *spc;
	int s;
	double r, dss, ccs;

	spc = (sampler_context *)ctx;


	s = (int)fpr_floor(mu);
	r = fpr_sub(mu, s);

	dss = fpr_half(fpr_sqr(isigma));


	ccs = fpr_mul(isigma, spc->sigma_min);

	for (;;) {
		int z0, z, b;
		double x;

		z0 = gaussian0_sampler(&spc->p);
		b = (int)prng_get_u8(&spc->p) & 1;
		z = b + ((b << 1) - 1) * z0;

		x = fpr_mul(fpr_sqr(fpr_sub(z, r)), dss);
		x = fpr_sub(x, fpr_mul(z0 * z0, fpr_inv_2sqrsigma0));
		if (BerExp(&spc->p, x, ccs)) {
			return s + z;
		}
	}
}

/* see inner.h */
void
sign_tree(int16_t *sig, inner_shake256_context *rng,
	const double * expanded_key,
	const uint16_t *hm, unsigned logn, uint8_t *tmp)
{
	double *ftmp;

	ftmp = (double *)tmp;
	for (;;) {
		sampler_context spc;
		samplerZ samp;
		void *samp_ctx;

		spc.sigma_min = fpr_sigma_min[logn];
		prng_init(&spc.p, rng);
		samp = sampler;
		samp_ctx = &spc;

		if (do_sign_tree(samp, samp_ctx, sig,
			expanded_key, hm, logn, ftmp))
		{
			break;
		}
	}
}

void
sign_dyn(int16_t *sig, inner_shake256_context *rng,
	const int8_t * f, const int8_t * g,
	const int8_t * F, const int8_t * G,
	const uint16_t *hm, unsigned logn, uint8_t *tmp)
{
	double *ftmp;

	ftmp = (double *)tmp;
	for (;;) {

		sampler_context spc;
		samplerZ samp;
		void *samp_ctx;

		spc.sigma_min = fpr_sigma_min[logn];
		prng_init(&spc.p, rng);
		samp = sampler;
		samp_ctx = &spc;

		if (do_sign_dyn(samp, samp_ctx, sig,
			f, g, F, G, hm, logn, ftmp))
		{
			break;
		}
	}
}
