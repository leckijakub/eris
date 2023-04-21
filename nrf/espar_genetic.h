#define POPULATION_SIZE 24
#define ELITE_SIZE 2
#define TOURNAMENT_SIZE 5
#define MUTATION_RATE 0.01
#define MAX_GENERATIONS 1000
#define GENE_NUMBER 12

#define EG_INVALID_GENOME (1<<GENE_NUMBER)

#include <stdint.h>
#include <stdbool.h>

// Struct to represent an individual in the population
typedef struct {
	int genes[GENE_NUMBER]; // Array of genes (i.e., binary elements)
	int8_t fitness;		// Fitness score of the individual
} Individual;

struct espar_gen_ctx {
	int8_t genome_fitness[1 << GENE_NUMBER];
	Individual population[POPULATION_SIZE];
	int (*fitness_func)(uint16_t genome);
};

void espar_gen_init(struct espar_gen_ctx *ctx);
void espar_gen_eval_generation(struct espar_gen_ctx *ctx);
void espar_gen_next_generation(struct espar_gen_ctx *ctx);
uint16_t espar_gen_next_genome_to_eval(struct espar_gen_ctx *ctx);
void espar_gen_record_fitness(struct espar_gen_ctx *ctx, uint16_t genome, int8_t fitness);
