#include "espar_genetic.h"

#include "nrf.h"
#include "nrf_drv_rng.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#define NRF_RAND_MAX ((1 << 31))

/**
 * @brief Function for generating an 8-bit random number with the internal
 * random generator.
 */
static uint32_t rnd8(void)
{
	uint8_t available;
	uint8_t random_byte;

	do {
		nrf_drv_rng_bytes_available(&available);
	} while (available < 0);
	nrf_drv_rng_rand(&random_byte, 1);
	return random_byte;
	// NRF_RNG->EVENTS_VALRDY = 0;

	// while (NRF_RNG->EVENTS_VALRDY == 0) {
	// 	// Do nothing.
	// }
	// return NRF_RNG->VALUE;
}

/**
 * @brief Function for generating a 32-bit random number with the internal
 * random generator.
 */
static uint32_t rand32(void)
{
	uint8_t i;
	uint32_t val = 0;

	for (i = 0; i < 4; i++) {
		val <<= 8;
		val |= rnd8();
	}
	// NRF_LOG_INFO("RAND %d", val);
	return val;
}

// Function to generate a random number between min and max (inclusive)
int rand_between(int min, int max)
{
	return (rand32() % (max - min + 1)) + min;
}

/**
 * @brief Genome to int - convert bool array of genes to int value
 *
 * @param genes array of genes
 * @param gen_len length of the array
 * @return uint16_t
 */
uint16_t genti(int genes[], size_t gen_len)
{
	uint16_t value = 0;
	for (int i = 0; i < gen_len; i++) {
		value += genes[i] << i;
	}
	return value;
}

// Function to initialize a new individual with random binary genes
void initialize_individual(Individual *individual)
{
	for (int i = 0; i < GENE_NUMBER; i++) {
		individual->genes[i] =
		    rand_between(0, 1); // Assuming binary elements are 0 or 1
	}
	individual->fitness = 0;
}

bool fitness_recorded(struct espar_gen_ctx *ctx, uint16_t genome)
{
	return ctx->genome_fitness[genome] != 0;
}

// Function to evaluate the fitness of an individual
void evaluate_fitness(struct espar_gen_ctx *ctx, Individual *individual)
{
	uint16_t genome = genti(individual->genes, GENE_NUMBER);
	int fitness = 0;
	if (ctx->genome_fitness[genome] > 0) {
		fitness = ctx->genome_fitness[genome];
		goto eval_fit_exit;
	}
	fitness = ctx->fitness_func(genome);
	ctx->genome_fitness[fitness] = fitness;
eval_fit_exit:
	individual->fitness = fitness;
}

// Function to select a parent using tournament selection
Individual *tournament_selection(Individual *population)
{
	Individual *best = &population[rand_between(0, POPULATION_SIZE - 1)];
	for (int i = 0; i < TOURNAMENT_SIZE - 1; i++) {
		Individual *candidate =
		    &population[rand_between(0, POPULATION_SIZE - 1)];
		if (candidate->fitness > best->fitness) {
			best = candidate;
		}
	}
	return best;
}

// Function to perform crossover between two parents
void crossover(Individual *parent1, Individual *parent2, Individual *child1,
	       Individual *child2)
{
	// Use single-point crossover
	int crossover_point =
	    rand_between(1, GENE_NUMBER - 1); // Choose a random crossover point
	for (int i = 0; i < GENE_NUMBER; i++) {
		if (i < crossover_point) {
			child1->genes[i] = parent1->genes[i];
			child2->genes[i] = parent2->genes[i];
		} else {
			child1->genes[i] = parent2->genes[i];
			child2->genes[i] = parent1->genes[i];
		}
	}
}

// Function to perform mutation on an individual
void mutate(Individual *individual)
{
	for (int i = 0; i < GENE_NUMBER; i++) {
		if ((double)rand32() / NRF_RAND_MAX <
		    MUTATION_RATE) { // Mutate with a certain probability
			individual->genes[i] =
			    1 - individual->genes[i]; // Flip the binary element
		}
	}
}

// Function to sort the population by fitness in descending order
void sort_population(Individual *population)
{
	Individual temp;
	for (int i = 0; i < POPULATION_SIZE - 1; i++) {
		for (int j = i + 1; j < POPULATION_SIZE; j++) {
			if (population[j].fitness > population[i].fitness) {
				// Swap the two individuals
				temp = population[i];
				population[i] = population[j];
				population[j] = temp;
			}
		}
	}
}

// Function to print the genes and fitness of the best individual in the
// population
void print_individual(Individual *individual)
{
	NRF_LOG_INFO("Individual: ");
	for (int i = 0; i < GENE_NUMBER; i++) {
		NRF_LOG_INFO("%d", individual->genes[i]);
	}
	NRF_LOG_INFO(" (fitness = %d)\n", individual->fitness);
}

void print_best_individual(Individual *population)
{
	NRF_LOG_INFO("Best individual: ");
	for (int i = 0; i < GENE_NUMBER; i++) {
		NRF_LOG_INFO("%d", population[0].genes[i]);
	}
	NRF_LOG_INFO(" (fitness = %d)\n", population[0].fitness);
}

void espar_gen_init(struct espar_gen_ctx *ctx)
{
	// // Seed the random number generator
	// srand32(time(NULL));
	// Initialize the population
	memset(ctx->genome_fitness, 0, sizeof(ctx->genome_fitness));
	for (int i = 0; i < POPULATION_SIZE; i++) {
		initialize_individual(&ctx->population[i]);
		NRF_LOG_INFO("Initial population %d : %d", i,
			     genti(ctx->population[0].genes, GENE_NUMBER));
		// NRF_LOG_INFO("Initial population:");
		// print_individual(&ctx->population[i]);
	}
	// NRF_LOG_HEXDUMP_INFO(ctx->population, sizeof(ctx->population));
}

void espar_get_update_fitness_from_store(struct espar_gen_ctx *ctx)
{
	for (int i = 0; i < POPULATION_SIZE; i++) {
		ctx->population[i].fitness = ctx->genome_fitness[genti(
		    ctx->population[i].genes, GENE_NUMBER)];
	}
}

void espar_gen_next_generation(struct espar_gen_ctx *ctx)
{
	NRF_LOG_INFO("CREATING NEW GENERATION");
	NRF_LOG_INFO("Genome 2 fitness: %d", ctx->population[2].fitness);
	// NRF_LOG_RAW_INFO
	// Sort the new population by fitness in descending order
	espar_get_update_fitness_from_store(ctx);
	sort_population(ctx->population);
	NRF_LOG_INFO("FIRST fitness: %d", ctx->population[0].fitness);

	// Print the best individual of the new population
	print_best_individual(ctx->population);

	Individual new_population[POPULATION_SIZE];

	// Copy the elite individuals from the old population to the new
	// one
	for (int i = 0; i < ELITE_SIZE; i++) {
		new_population[i] = ctx->population[i];
	}

	// Fill the rest of the new population through selection,
	// crossover, and mutation
	for (int i = ELITE_SIZE; i < POPULATION_SIZE; i += 2) {
		// Select two parents using tournament selection
		Individual *parent1 = tournament_selection(ctx->population);
		Individual *parent2 = tournament_selection(ctx->population);

		// Create a child through crossover
		Individual child1;
		Individual child2;
		crossover(parent1, parent2, &child1, &child2);

		// Mutate the child
		mutate(&child1);
		mutate(&child2);

		// Add the child to the new population
		new_population[i] = child1;
		new_population[i + 1] = child2;
	}

	// Replace the old population with the new one
	for (int i = 0; i < POPULATION_SIZE; i++) {
		ctx->population[i] = new_population[i];
	}
}
void espar_gen_eval_generation(struct espar_gen_ctx *ctx)
{
	// // Iterate until the maximum number of generations is reached
	// for (int generation = 1; generation <= MAX_GENERATIONS; generation++)
	// { Create a new population of the same size as the old one

	// Evaluate the fitness of each individual in the new population
	for (int i = 0; i < POPULATION_SIZE; i++) {
		evaluate_fitness(ctx, &ctx->population[i]);
	}

	// Sort the new population by fitness in descending order
	sort_population(ctx->population);

	// Print the best individual of the new population
	print_best_individual(ctx->population);

	// }
}

uint16_t espar_gen_next_genome_to_eval(struct espar_gen_ctx *ctx)
{
	uint16_t genome;
	for (int i = 0; i < POPULATION_SIZE; i++) {
		genome = genti(ctx->population[i].genes, GENE_NUMBER);
		if (!fitness_recorded(ctx, genome)) {
			NRF_LOG_INFO("next genome: %d, id: %d", genome, i);
			return genome;
		}
	}
	return EG_INVALID_GENOME;
}

void espar_gen_record_fitness(struct espar_gen_ctx *ctx, uint16_t genome,
			      int8_t fitness)
{
	ctx->genome_fitness[genome] = fitness;
	NRF_LOG_INFO("Recording genome %d fitness: %d", genome, fitness);
	for (int i = 0; i < POPULATION_SIZE; i++) {
		if (genti(ctx->population[i].genes, GENE_NUMBER) == genome) {
			ctx->population[i].fitness = fitness;
			// NRF_LOG_INFO("Recording genome: %d, fitness: %d",
			// genome, fitness);
			return;
		}
	}
}
