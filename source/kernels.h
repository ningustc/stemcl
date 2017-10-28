const char *kernels_ocl =
"// Propagator layout: Stack of nlayer 2d arrays of nx by ny float4 values: xr, xi, yr, yi\n"
"kernel void propagate(global float2 *complex_data, global const float2* propagator_x, global const float2* propagator_y, global float* kx2, global float* ky2, float k2max, int slice) {\n"
"    size_t ix = get_global_id(0);\n"
"    size_t iy = get_global_id(1);\n"
"    size_t index = iy * get_global_size(0) + ix;\n"
"    float pxr, pxi, pyr, pyi;\n"
"    float2 w, t;\n"
"    \n"
"    if( kx2[ix] < k2max ) {\n"
"        pxr = propagator_x[ix].s0;\n"
"        pxi = propagator_x[ix].s1;\n"
"\n"
"        if( (kx2[ix] + ky2[iy]) < k2max ) {\n"
"            pyr = propagator_y[iy].s0;\n"
"            pyi = propagator_y[iy].s1;\n"
"            w = complex_data[index];\n"
"            t.s0 = w.s0*pyr - w.s1*pyi;\n"
"            t.s1 = w.s0*pyi + w.s1*pyr;\n"
"            complex_data[index].s0 = t.s0*pxr - t.s1*pxi;\n"
"            complex_data[index].s1 = t.s0*pxi + t.s1*pxr;\n"
"        } else {\n"
"            complex_data[index] = 0.0f;\n"
"        }\n"
"        \n"
"    } else {\n"
"        complex_data[index] = 0.0f;\n"
"    }\n"
"    \n"
"}\n"
"\n"
"kernel void transmit (global float2 *complex_data, global float2 *transmission_function, int ixoff, int iyoff, int trans_nx, int trans_ny) {\n"
"    int ix = get_global_id(0);\n"
"    int iy = get_global_id(1);\n"
"    int index = iy * get_global_size(0) + ix;\n"
"    int ixt, iyt;\n"
"    \n"
"    ixt = ix + ixoff;\n"
"    if (ixt >= trans_nx) {\n"
"        ixt = ixt - trans_nx;\n"
"    } else if (ixt < 0) {\n"
"        ixt = ixt + trans_nx;\n"
"    }\n"
"    \n"
"    iyt = iy + iyoff;\n"
"    if (iyt >= trans_ny) {\n"
"        iyt = iyt - trans_ny;\n"
"    } else if (iyt < 0) {\n"
"        iyt = iyt + trans_ny;\n"
"    }\n"
"    \n"
"    size_t index_trans = iyt * trans_nx + ixt;\n"
"    float2 pr = complex_data[index];\n"
"    float2 tr = transmission_function[index_trans];\n"
"    pr.s0 = pr.s0*tr.s0 - pr.s1*tr.s1;\n"
"    pr.s1 = pr.s0*tr.s1 + pr.s1*tr.s0;\n"
"    complex_data[index] = pr;\n"
"}\n"
"\n"
"// temp has to be a buffer of at least probe-x-dimension * n_detect * sizeof(float)\n"
"kernel void sum_intensity(global float2 *probe, global float *kxp2, global float *kyp2, global float *k2min, global float *k2max, int n_detect, int probe_ny, global float *temp) {\n"
"    int ix = get_global_id(0);\n"
"    int probe_nx = get_global_size(0);\n"
"    \n"
"    // Initialize output for all detectors\n"
"    for(int idetect=0; idetect<n_detect; idetect++) {\n"
"        temp[idetect * probe_nx + ix] = 0;\n"
"    }\n"
"    \n"
"    for (int iy=0; iy<probe_ny; iy++) {\n"
"        float prr = probe[iy*probe_nx + ix].s0;\n"
"        float pri = probe[iy*probe_nx + ix].s1;\n"
"        float absolute = prr*prr + pri*pri;\n"
"        float k2 = kxp2[ix] + kyp2[iy];\n"
"        \n"
"        for(int idetect=0; idetect<n_detect; idetect++) {\n"
"\n"
"            if ((k2 >= k2min[idetect]) && (k2 <= k2max[idetect])) {\n"
"                temp[idetect * probe_nx + ix] += absolute;\n"
"            }\n"
"            \n"
"        }\n"
"        \n"
"    }\n"
"    \n"
"}\n"
"\n"
"kernel void prepare_probe_espread(float x, float y, float k2maxa, float k2maxb, float chi1, float chi2, float chi3, float xoff, float yoff, global float2 *probe_data, float wavlen, float dfa2, float dfa2phi, float dfa3, float dfa3phi, float df, global float *p_spat_freqx, global float *p_spat_freqy, global float *p_spat_freqx2, global float *p_spat_freqy2, global float *spat_freqx, global float *spat_freqy) {\n"
"    \n"
"    size_t ix = get_global_id(0);\n"
"    size_t iy = get_global_id(1);\n"
"    size_t index = iy * get_global_size(0) + ix;\n"
"    \n"
"    size_t nx = get_global_size(0);\n"
"    size_t ny = get_global_size(1);\n"
"    \n"
"    float2 pd = probe_data[index];    \n"
"    float scale = 1.0/sqrt((float)nx*(float)ny);\n"
"    \n"
"    float pi = 4.0f * atan( 1.0f );\n"
"    \n"
"    float k2 = p_spat_freqx2[ix] + p_spat_freqy2[iy];\n"
"    \n"
"    float ktheta2 = k2*(wavlen*wavlen);\n"
"    float w = 2.0f*pi* ( xoff*p_spat_freqx[ix] + yoff*p_spat_freqy[iy] );\n"
"    float phi = atan2( spat_freqy[iy], spat_freqx[ix] );\n"
"    \n"
"    float chi = ktheta2*(-df+chi2 + dfa2*sin(2.0f*(phi-dfa2phi)))/2.0f;\n"
"    chi *= 2*pi/wavlen;\n"
"    chi -= w;\n"
"    \n"
"    if (fabs(k2-k2maxa) <= chi1) {\n"
"		pd.s0 = 0.5 * scale * cos(chi);\n"
"		pd.s1 = -0.5 * scale * sin(chi);\n"
"	} else if( (k2 >= k2maxa) && (k2 <= k2maxb) ) {\n"
"	    pd.s0 = scale * cos( chi );  /* real */\n"
"        pd.s1 = -scale * sin( chi );  /* imag */\n"
"    } else {\n"
"        pd.s0 = 0.0f;\n"
"        pd.s1 = 0.0f;\n"
"    }\n"
"    \n"
"    probe_data[index] = pd;\n"
"}\n"
"\n"
"kernel void prepare_probe(float x, float y, float k2maxa, float k2maxb, float chi1, float chi2, float chi3, float xoff, float yoff, global float2 *probe_data, float wavlen, float dfa2, float dfa2phi, float dfa3, float dfa3phi, float df, global float *p_spat_freqx, global float *p_spat_freqy, global float *p_spat_freqx2, global float *p_spat_freqy2, global float *spat_freqx, global float *spat_freqy) {\n"
"    \n"
"    size_t ix = get_global_id(0);\n"
"    size_t iy = get_global_id(1);\n"
"    size_t index = iy * get_global_size(0) + ix;\n"
"    \n"
"    float pi = 4.0f * atan( 1.0f );\n"
"    \n"
"    float2 pd = probe_data[index];\n"
"    \n"
"    float k2 = p_spat_freqx2[ix] + p_spat_freqy2[iy];\n"
"    if( (k2 >= k2maxa) && (k2 <= k2maxb) ) {\n"
"        float w = 2.0f*pi* ( xoff*p_spat_freqx[ix] + yoff*p_spat_freqy[iy] );\n"
"        float phi = atan2( spat_freqy[iy], spat_freqx[ix] );\n"
"        float chi = chi1*k2* ( (chi2 + chi3*k2)*k2 - df\n"
"                              + dfa2*sin( 2.0f*(phi-dfa2phi) )\n"
"                              + 2.0f * dfa3 * wavlen * sqrt(k2)*\n"
"                              sin( 3.0f*(phi-dfa3phi) )/3.0f );\n"
"        chi= - chi + w;\n"
"        \n"
"        pd.s0 = (float) cos( chi );\n"
"        pd.s1 = (float) sin( chi );\n"
"    } else {\n"
"        pd.s0 = 0.0f;\n"
"        pd.s1 = 0.0f;\n"
"    }\n"
"    probe_data[index] = pd;\n"
"}\n"
"\n"
"kernel void scale_probe(global float2 *probe_data, float probe_sum) {\n"
"    float scale = (float) ( 1.0f/sqrt(probe_sum) );\n"
"    \n"
"    size_t ix = get_global_id(0);\n"
"    size_t iy = get_global_id(1);\n"
"    size_t index = iy * get_global_size(0) + ix;\n"
"    \n"
"    float2 pd2 = probe_data[index];\n"
"    pd2.s0 *= scale;\n"
"    pd2.s1 *= scale;\n"
"    probe_data[index] = pd2;\n"
"}\n"
"\n"
"kernel void gauss_multiply(global float2 *probe_data, float gausscale) {\n"
"\n"
"	size_t ix = get_global_id(0);\n"
"    size_t iy = get_global_id(1);\n"
"    size_t index = iy * get_global_size(0) + ix;\n"
"    \n"
"    size_t nx = get_global_size(0);\n"
"    size_t ny = get_global_size(1);\n"
"    \n"
"    float2 pd = probe_data[index]; \n"
"\n"
"	float r = exp(-((ix-nx/2.0f)*(ix-nx/2.0f)+(iy-ny/2.0f)*(iy-ny/2.0f))/((float)nx*(float)ny*gausscale));\n"
"	pd.s0 *= r;\n"
"	pd.s1 *= r;\n"
"		\n"
"	probe_data[index] = pd;\n"
"}\n"
;
