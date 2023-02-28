/* @file main.c
**
******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <slow5/slow5.h>
#include "sigfish.h"
#include "rjnn.h"
#include "error.h"
#include "misc.h"


float **get_chunks(const float *raw, int64_t nsample, int chunk_size, int num_chunks){
    float **chunks = (float **)malloc(sizeof(float *)*num_chunks);
    MALLOC_CHK(chunks);
    for (int chunk_i=0; chunk_i<num_chunks; chunk_i++){
        chunks[chunk_i] = (float *)malloc(sizeof(float)*chunk_size);
        MALLOC_CHK(chunks[chunk_i]);
        int cur_chunk_size = (chunk_i == num_chunks-1) ? nsample - chunk_i*chunk_size : chunk_size;
        for(int j=0; j<cur_chunk_size; j++){
            chunks[chunk_i][j] = raw[chunk_i*chunk_size + j];
            assert(chunk_i*chunk_size + j < nsample);
        }
    }
    return chunks;
}

void jnn_v3(const float *raw, int64_t nsample, jnnv3_aparam_t param, jnnv3_astate_t *s, jnnv3_pparam_t pparam, jnnv3_pstate_t *t, refsynth_t *ref, char *read_id){

    // now feed algorithm with chunks of signal simulating real-time
    const int chunk_size = 1200;
    const int start_chunks = 6; //num of chunks to store before processing
    const int num_chunks = (nsample + chunk_size -1) / chunk_size;
    float **chunks = get_chunks(raw, nsample, chunk_size, num_chunks);

    float *sig_store = (float *)malloc(sizeof(float)*nsample);
    MALLOC_CHK(sig_store);
    int sig_store_i = 0;

    // this is the algo. Simple yet effective
    reset_jnnv3_astate(s,param);
    reset_jnnv3_pstate(t,pparam);
    aln_t best_aln = {0};
    best_aln.pos_st = -1;
    char *paf = NULL;

    for (int chunk_i=0; chunk_i < num_chunks; chunk_i++){

        // print("chunk {}".format(chunk))
        int current_chunk_size = (chunk_i == num_chunks-1) ? nsample - chunk_i*chunk_size : chunk_size;
        float *chunk = chunks[chunk_i];
        //fprintf(stderr,"processing chunk: %d, size %d\n", chunk_i, current_chunk_size);

        for(int j=0; j<current_chunk_size; j++){
            sig_store[sig_store_i] = chunk[j];
            sig_store_i++;
            assert(sig_store_i <= nsample);
        }

        if (chunk_i < start_chunks){ //nothing
            continue;
            //fprintf(stderr,"chunk_i: %d added to sig_store\n", chunk_i);

        }
        //fprintf(stderr,"size_i: %d\n", sig_store_i);
        if (chunk_i == start_chunks){ //enough chunks arrived
            int sig_size = sig_store_i;
            //sig_store_i = 0;
            jnnv3_acalc_param(s, param, sig_store, sig_size);
            //fprintf(stderr,"s->median: %f, s->stdev: %f, s->top: %f\n", s->median, s->stdev, s->top);
            chunk = sig_store;
            current_chunk_size = sig_size;
        }

        if (!s->adapter_found){
            jnnv3_acore(s, param, chunk, current_chunk_size);
            if (s->adapter_found){
                jnn_pair_t p = s->segs[0];
                jnnv3_pcalc_param(t,p,sig_store,sig_store_i);
                chunk = &sig_store[p.y];
                current_chunk_size = sig_store_i-p.y;

            }
        }

        if(s->adapter_found && !t->polya_found){
            jnnv3_pcore(t, pparam,chunk,current_chunk_size);
        }

        if(t->polya_found){
            assert(s->adapter_found == 1);
            assert(t->seg_i > 0);
            assert(s->seg_i > 0);
            jnn_pair_t polya = t->segs[0];
            jnn_pair_t adapt = s->segs[0];
            int st = polya.y+adapt.y-1;
            int leftover = sig_store_i - st;
            if(leftover >= QUERY_SIZE_SIG){
                //fprintf(stderr,"leftover: %d, running DTW\n", leftover);
                if(ref){
                    best_aln=map(ref, sig_store, sig_store_i, st, read_id, &paf);
                }
                break;
            } else {
                //fprintf(stderr,"leftover: %d, waiting for more\n", leftover);
            }

        }

    }

    if(ref==NULL){
        if(s->seg_i<=0){
            assert(s->adapter_found == 0);
            printf(".\t.\t");
        } else{
            printf("%ld\t%ld\t",s->segs[0].x,s->segs[0].y);
        }
        if(t->seg_i <= 0){
            assert(t->polya_found == 0);
            printf(".\t.\n");
        } else {
            jnn_pair_t polya = t->segs[0];
            assert(polya.y + s->segs[0].y < sig_store_i);
            printf("%ld\t%ld\n",polya.x+s->segs[0].y-1, polya.y+s->segs[0].y-1);
        }
    }else{
        if(best_aln.pos_st > 0){
            printf("%s",paf);
            free(paf);
        }
    }


    for (int i=0; i<num_chunks; i++){
        free(chunks[i]);
    }
    free(chunks);
    free(sig_store);

}



int real_main1(int argc, char* argv[]){

    if(argc < 2){
        fprintf(stderr,"Usage: sigfish %s <file.blow5>\n", argv[0]);
        fprintf(stderr,"Usage: sigfish %s <file.blow5> <ref.fa>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    slow5_file_t *sp = slow5_open(argv[1],"r");
    if(sp==NULL){
       fprintf(stderr,"Error in opening file\n");
       exit(EXIT_FAILURE);
    }
    slow5_rec_t *rec = NULL;
    int ret=0;

    refsynth_t *ref = NULL;
    if(argc == 3){
        const char *ref_name = argv[2];
        assert(ref_name != NULL);
        model_t *pore_model = (model_t*)malloc(sizeof(model_t) * MAX_NUM_KMER); //4096 is 4^6 which is hardcoded now
        MALLOC_CHK(pore_model);
        uint32_t kmer_size = set_model(pore_model, MODEL_ID_RNA_NUCLEOTIDE);
        uint32_t flag = 0;
        flag |= SIGFISH_RNA;
        int32_t query_size = QUERY_SIZE_EVENTS;
        ref = gen_ref(ref_name, pore_model, kmer_size, flag, query_size);
        free(pore_model);

    }

    if(ref == NULL) printf("read_id\tlen_raw_signal\tadapt_start\tadapt_end\tpolya_start\tpolya_end\n");

    const jnnv3_aparam_t param = JNNV3_ADAPTOR;
    jnnv3_astate_t *s= init_jnnv3_astate(param);
    const jnnv3_pparam_t pparam = JNNV3_POLYA;
    jnnv3_pstate_t *t = init_jnnv3_pstate(pparam);

    while((ret = slow5_get_next(&rec,sp)) >= 0){
        if(ref == NULL) printf("%s\t%ld\t",rec->read_id,rec->len_raw_signal);
        float *signal = signal_in_picoamps(rec);
        jnn_v3(signal, rec->len_raw_signal, param, s, pparam, t, ref, rec->read_id);
        free(signal);
    }

    if(ret != SLOW5_ERR_EOF){  //check if proper end of file has been reached
        fprintf(stderr,"Error in slow5_get_next. Error code %d\n",ret);
        exit(EXIT_FAILURE);
    }

    free_jnnv3_astate(s);
    free_jnnv3_pstate(t);
    slow5_rec_free(rec);
    slow5_close(sp);

    if(ref != NULL){
        free_ref(ref);
    }

    return 0;
}

#define TO_PICOAMPS(RAW_VAL,DIGITISATION,OFFSET,RANGE) (((RAW_VAL)+(OFFSET))*((RANGE)/(DIGITISATION)))

int real_main2(int argc, char* argv[]){

    if(argc < 3){
        fprintf(stderr,"Usage: sigfish %s <file.blow5> <ref.fa>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    slow5_file_t *sp = slow5_open(argv[1],"r");
    if(sp==NULL){
       fprintf(stderr,"Error in opening file\n");
       exit(EXIT_FAILURE);
    }
    slow5_rec_t *rec = NULL;
    int ret=0;

    int channels=512;

    sigfish_opt_t opt;
    opt.num_thread = 8;
    opt.debug_paf = "-";
    opt.no_full_ref = 1;
    sigfish_state_t *state = init_sigfish(argv[2], channels, opt);
    sigfish_read_t reads[channels];

    int round=0;
    while (ret==0){
        int i = 0;
        while (i < channels && (ret = slow5_get_next(&rec,sp))>= 0) {
            reads[i].channel = i+1;
            reads[i].read_number = round;
            reads[i].len_raw_signal = rec->len_raw_signal;
            reads[i].read_id = NULL;
            reads[i].raw_signal = (float*)malloc(sizeof(float)*rec->len_raw_signal);
            for (int j=0; j<rec->len_raw_signal; j++){
                reads[i].raw_signal[j] = TO_PICOAMPS(rec->raw_signal[j],rec->digitisation,rec->offset,rec->range);
            }
            i++;
        }
        fprintf(stderr,"round %d: %d reads loaded\n",round,i);

        enum sigfish_status *status = process_sigfish(state, reads, channels);
        for(int j=0;j<i;j++){
            printf("channel %d: %d\n",j+1,status[j]);
        }
        free(status);
        for(int j=0; j<i; j++){
            free(reads[j].raw_signal);
        }
        fprintf(stderr,"round %d: %d reads processed\n",round,i);
        round++;

    }

    free_sigfish(state);
    slow5_rec_free(rec);
    slow5_close(sp);

    return 0;

}

int real_main(int argc, char* argv[]){

    real_main1(argc, argv);

    return 0;
}