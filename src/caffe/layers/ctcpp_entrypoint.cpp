#include <cstddef>
#include <iostream>
#include <algorithm>

#include "caffe/ctc/ctcpp.hpp"
#include "caffe/ctc/cpu_ctc.h"


namespace CTC {

int get_warpctc_version() {
    return 2;
}

const char* ctcGetStatusString(ctcStatus_t status) {
	switch (status) {
	case CTC_STATUS_SUCCESS:
		return "no error";
	case CTC_STATUS_MEMOPS_FAILED:
		return "cuda memcpy or memset failed";
	case CTC_STATUS_INVALID_VALUE:
		return "invalid value";
	case CTC_STATUS_EXECUTION_FAILED:
		return "execution failed";

	case CTC_STATUS_UNKNOWN_ERROR:
	default:
		return "unknown error";

	}

}

template<typename Dtype>
ctcStatus_t compute_ctc_loss_cpu(const Dtype* const activations,
                             Dtype* gradients,
                             const int* const flat_labels,
                             const int* const label_lengths,
                             const int* const input_lengths,
                             int alphabet_size,
                             int minibatch,
                             Dtype *costs,
                             void *workspace,
                             ctcOptions options) {

    if (activations == nullptr ||
        flat_labels == nullptr ||
        label_lengths == nullptr ||
        input_lengths == nullptr ||
        costs == nullptr ||
        workspace == nullptr ||
        alphabet_size <= 0 ||
        minibatch <= 0)
        return CTC_STATUS_INVALID_VALUE;

    if (options.loc == CTC_CPU) {
        CpuCTC<Dtype> ctc(alphabet_size, minibatch, workspace, options.num_threads,
                          options.blank_label);

        if (gradients != NULL)
            return ctc.cost_and_grad(activations, gradients,
                                     costs,
                                     flat_labels, label_lengths,
                                     input_lengths);
        else
            return ctc.score_forward(activations, costs, flat_labels,
                                     label_lengths, input_lengths);
    } else if (options.loc == CTC_GPU) {

    } else {
        return CTC_STATUS_INVALID_VALUE;
    }
	return CTC_STATUS_SUCCESS;
}


template <typename Dtype>
ctcStatus_t get_workspace_size(const int* const label_lengths,
                               const int* const input_lengths,
                               int alphabet_size, int minibatch,
                               ctcOptions options,
                               size_t* size_bytes)
{
    if (label_lengths == nullptr ||
        input_lengths == nullptr ||
        size_bytes == nullptr ||
        alphabet_size <= 0 ||
        minibatch <= 0)
        return CTC_STATUS_INVALID_VALUE;

    // This is the max of all S and T for all examples in the minibatch.
    int maxL = *std::max_element(label_lengths, label_lengths + minibatch);
    int maxT = *std::max_element(input_lengths, input_lengths + minibatch);

    const int S = 2 * maxL + 1;

    *size_bytes = 0;

    if (options.loc == CTC_GPU) {
        // GPU storage
        //nll_forward, nll_backward
        *size_bytes += 2 * sizeof(Dtype) * minibatch;

        //repeats
        *size_bytes += sizeof(int) * minibatch;

        //label offsets
        *size_bytes += sizeof(int) * minibatch;

        //utt_length
        *size_bytes += sizeof(int) * minibatch;

        //label lengths
        *size_bytes += sizeof(int) * minibatch;

        //labels without blanks - overallocate for now
        *size_bytes += sizeof(int) * maxL * minibatch;

        //labels with blanks
        *size_bytes += sizeof(int) * S * minibatch;

        //alphas
        *size_bytes += sizeof(Dtype) * S * maxT * minibatch;

        //denoms
        *size_bytes += sizeof(Dtype) * maxT * minibatch;

        //probs (since we will pass in activations)
        *size_bytes += sizeof(Dtype) * alphabet_size * maxT * minibatch;

    } else {
        //cpu can eventually replace all minibatch with
        //max number of concurrent threads if memory is
        //really tight

        //per minibatch memory
        size_t per_minibatch_bytes = 0;

        //output
        per_minibatch_bytes += sizeof(Dtype) * alphabet_size ;

        //alphas
        per_minibatch_bytes += sizeof(Dtype) * S * maxT;

        //betas
        per_minibatch_bytes += sizeof(Dtype) * S;

        //labels w/blanks, e_inc, s_inc
        per_minibatch_bytes += 3 * sizeof(int) * S;

        *size_bytes = per_minibatch_bytes * minibatch;

        //probs
        *size_bytes += sizeof(Dtype) * alphabet_size * maxT * minibatch;
    }

    return CTC_STATUS_SUCCESS;
}
 
 template
 ctcStatus_t compute_ctc_loss_cpu<float>(const float* const activations,
                              float* gradients,
                              const int* const flat_labels,
                              const int* const label_lengths,
                              const int* const input_lengths,
                              int alphabet_size,
                              int minibatch,
                              float *costs,
                              void *workspace,
                              ctcOptions options);
 
 
 template
 ctcStatus_t compute_ctc_loss_cpu<double>(const double* const activations,
                              double* gradients,
                              const int* const flat_labels,
                              const int* const label_lengths,
                              const int* const input_lengths,
                              int alphabet_size,
                              int minibatch,
                              double *costs,
                              void *workspace,
                              ctcOptions);
 
 
 template
 ctcStatus_t get_workspace_size<float>(const int* const label_lengths,
                                const int* const input_lengths,
                                int alphabet_size, int minibatch,
                                ctcOptions,
                                size_t* size_bytes);
 
 
 template
 ctcStatus_t get_workspace_size<double>(const int* const label_lengths,
                                const int* const input_lengths,
                                int alphabet_size, int minibatch,
                                ctcOptions,
                                size_t* size_bytes);

}  // namespace ctc

