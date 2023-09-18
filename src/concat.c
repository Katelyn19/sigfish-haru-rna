#include <stdio.h>
#include "sigfish.h"
#include "error.h"

int concat_main(int argc) {
    char* fastafile = "./test/RNA/refs/rnasequin_sequences_2.4.fa";
    char *slow5file = "test/RNA/reads/sequin_rna.blow5";

    double realtime0 = 0;

    opt_t opt;
    init_opt(&opt);

    core_t* core = init_core(fastafile, slow5file, opt, realtime0);

    int ref_len_total = 0;

    for (int i = 0; i < core->ref->num_ref; i++) {
        ref_len_total += core->ref->ref_lengths[i];
    }

    INFO("Total Refs: %d, Ref_Len_Total: %d", core->ref->num_ref, ref_len_total);

    int32_t *ref = (int32_t *)malloc(sizeof(int32_t) * ref_len_total );
    MALLOC_CHK(ref);

    for (int i = 0; i < core->ref->num_ref; i++) {
        for (int j = 0; j < core->ref->ref_lengths[i]; j++) {
            ref[i] = core->ref->forward[i][j];
        }
    }

    int pos = 5600;
    int tmp_total = 0;
    int cumulative = -1;
    int ref_id = -1;
    for (int i = 0; i < core->ref->num_ref; i++) {
        tmp_total += core->ref->ref_lengths[i];
        
        if (tmp_total > pos && ref_id < 0) {
            ref_id = i;
            cumulative = tmp_total - core->ref->ref_lengths[i];
        }
    }

    if (ref_id < 0) {
        INFO("%s", "Error");
    }

    int actual_pos = pos - cumulative;

    INFO("pos: %d, ref: %d, actual pos: %d", pos, ref_id, actual_pos);
}