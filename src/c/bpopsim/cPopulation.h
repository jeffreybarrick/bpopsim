#ifndef cPopulation_h
#define cPopulation_h

#include "common.h"
#include "cSubpopulation.h"

using namespace std;

namespace bpopsim {


  // Statistics for a single run
  struct cReplicateStatistics {
    
    vector<double> average_population_fitness;
    
    vector< vector<cGenotypeFrequency> > genotype_frequencies;  // Frequencies of each genotype
    vector< vector<cGenotypeFrequency> > clade_frequencies;     // Frequencies of each clade with the given genotype as its ancestor
    
    uint32_t total_mutations;                                   // Number of mutations so far
    uint32_t total_subpopulations_lost;                         // Number of subpopulations that no longer exist
    uint32_t num_subpopulations_culled;
    vector< vector<double> > red_white_ratios;
    
  };
  
  // Statistics across all runs
  struct cStatistics : vector<cReplicateStatistics> {

    cStatistics(AnyOption& options)
    {
      output_directory_name = options["output-folder"];
      coarse_graining = from_string<uint32_t>(options["coarse-graining"]);
    }
    
    void OutputAveragePopulationFitness();

    string output_directory_name;
    uint32_t coarse_graining;   // Only used for calculating statistics and ignoring intermediate time points

  };
  
    
  class cPopulation {

  private:
    
    // Simulation parameters that should be arguments
    // Initialized outside of constructor
    
    class SimulationParameters {
      
    public:
      // Sort function
      bool by_numerical_value (double i, double j) { return (i<j); }
      
      SimulationParameters(AnyOption& options, gsl_rng* rng)
      { 
        // Populate input values
        maximum_number_of_transfers = from_string<uint32_t>(options["number-of-transfers"]);
        initial_population_size = from_string<double>(options["initial-population-size"]);
        initial_population_size_after_transfer = from_string<double>(options["population-size-after-transfer"]);
        generations_per_transfer = from_string<double>(options["generations-per-transfer"]);

        initial_fitness = from_string<double>(options["initial-fitness"]);
        mutation_rates_per_division = from_string<vector<double> >(options["mutation-rates"]);
        mutation_fitness_effects = from_string<vector<double> >(options["fitness-effects"]);
        mutation_fitness_effect_model = options["fitness-effect-model"];
        
        if (mutation_rates_per_division.size() != mutation_fitness_effects.size()) {
          
          options.addUsage("");
          options.addUsage("The number of mutation rates (-u) is not the same as the number of fitness effects (-s).");
          options.printUsage();
          exit(-1);
        }
        
        if (options.count("first-mutation-fitness-effects"))
          first_mutation_fitness_effects = from_string<vector< double > >(options["first-mutation-fitness-effects"]);
        
        // Populate calculated values
        transfer_dilution_factor = exp(log(2) * generations_per_transfer);
        final_population_size_at_transfer = transfer_dilution_factor * initial_population_size_after_transfer;
        binomial_sampling_transfer_probability = 1.0 / transfer_dilution_factor;
        
        // Sum mutation rates
        total_mutation_rate_per_division = 0;
        for(vector<double>::iterator it=mutation_rates_per_division.begin(); it!=mutation_rates_per_division.end(); it++)
          total_mutation_rate_per_division += *it;
        
        total_mutation_rate_exponential_mean = 1.0 / total_mutation_rate_per_division;
        
        // Fill in the fractional table for deciding which category mutations belong in.
        double on_mutation_rate_per_division = 0;
        for(vector<double>::iterator it=mutation_rates_per_division.begin(); it!=mutation_rates_per_division.end(); it++) {
          on_mutation_rate_per_division += *it;
          fractional_chances_of_mutation_categories.push_back(on_mutation_rate_per_division/total_mutation_rate_per_division);
        }
      
        // Exact mutation number - may want to implement within transfer loop, so divisions change. This way now for MA usage.
        /*
        exact_mutations_per_transfer = from_string<uint32_t>(options["exact-mutations-per-transfer"]);        
        double total_divisions_per_transfer = final_population_size_per_transfer - initial_population_size_per_transfer;
        for(uint32_t i=0; i<exact_mutations_per_transfer; i++) {
          double division_of_mutation = (double) trunc(gsl_rng_uniform_int(rng, total_divisions_per_transfer));
          exact_mutation_at_division.push_back( division_of_mutation );
        }
         sort(exact_mutation_at_division.begin(), exact_mutation_at_division.end());
        sort(exact_mutation_at_division.begin(), exact_mutation_at_division.end(), by_numerical_value);
        
        //exact_mutations_until_division.push_back( division_of_mutation );
        */
        
      }
      
      void Print() {
        cerr << "***** Simulation Settings *****" << endl;
        cerr << "  Initial population size                  (-p) = " << initial_population_size << endl;
        cerr << "  Generations of growth per transfer       (-T) = " << generations_per_transfer << endl;
        cerr << "  Transfer dilution                             = " << transfer_dilution_factor << endl;
        cerr << "  Population size after transfer           (-N) = " << initial_population_size_after_transfer << endl;
        cerr << "  Population size at transfer                   = " << final_population_size_at_transfer << endl;
        cerr << "  Mutation rates per division              (-u) = " << to_string(mutation_rates_per_division) << endl;
        cerr << "  Mutation fitness effects                 (-s) = " << to_string(mutation_fitness_effects) << endl;
        cerr << "  Mutation fitness effect model            (-f) = " << mutation_fitness_effect_model << endl;
      }
      
      // Input
      double maximum_number_of_transfers;               // Maximum number of transfers before ending simulation. May end earlier for other reasons.
      double initial_population_size;                  // Initial population size at the beginning of the simulation
      double initial_population_size_after_transfer;   // Initial population size after transfer dilution
      double generations_per_transfer;                  // Number of binary cell divisions per transfer
      
      double initial_fitness;                           // Fitness of cells at the beginning of the simulation
      vector<double> mutation_rates_per_division;       // Rate per cell division of mutations in each category
      vector<double> mutation_fitness_effects;          // Effect of mutations in each category = selection coefficient for uniform model 
      string mutation_fitness_effect_model;             // Model for drawing fitness effects: 'u' = uniform NOT FULLY IMPLEMENTED
      vector<double> first_mutation_fitness_effects; // NOT FULLY IMPLEMENTED
      
      // Calculated from Input 
      double transfer_dilution_factor;                  // The transfer dilution fraction
      double final_population_size_at_transfer;         // Final population size that triggers a transfer
      double binomial_sampling_transfer_probability;    // Probability for binomial sampling
      double total_mutation_rate_exponential_mean;              // Mean for Poisson draws of divisions until next mut
      double total_mutation_rate_per_division;                  // Sum of all mutation rates
      vector<double> fractional_chances_of_mutation_categories; // Fraction of mutations in each category
      
      /* Finish re-implementing
      uint32_t exact_mutations_per_transfer;             // Exact number of mutations to uniformly distribute during transfer
      vector<double> exact_mutation_at_division;
      vector<double> exact_mutation_next_division_interval;
      */

    } simulation_parameters;
    
    struct OutputParameters {
      
      OutputParameters(AnyOption& options)
      {
        output_directory_name = options["output-folder"];
        coarse_graining = from_string<uint32_t>(options["coarse-graining"]);
        burn_in = from_string<uint32_t>(options["burn-in"]);
      }
      
      string output_directory_name;
      uint32_t coarse_graining;
      int32_t burn_in;            // Number of transfers to perform before recording output
      uint32_t m_muller_rez;      // Vertical Resolution of Muller plot... set at command line (default: 2500)

    } output_parameters;
    
    struct MarkerDivergence {
      
      MarkerDivergence(AnyOption& options)
      { 
        // Populate input values
        marker_states = from_string<uint32_t>(options["marker-states"]);
        max_divergence_factor = from_string<double>(options["max-marker-divergence-ratio"]);
        transfer_interval_to_print = from_string<double>(options["transfer-interval-to-print"]);
      }
      
      bool marker_states;
      double max_divergence_factor; 
      double transfer_interval_to_print;

    } marker_divergence;
  
  public:
    cReplicateStatistics replicate_statistics;

  private:
    
    // Internal state variables
    bool debug;
    uint32_t replicate;
    
    // Tracks characteristics of all subpopulations (including location in Tree)
    vector<cSubpopulation> current_subpopulations;
    
    // Tree that tracks ancestry of the subpopulations
    tree<cGenotype> genotype_tree;
    uint32_t unique_genotype_count;            // used to assign node ids in tree, should equal number of nodes that ever existed
    uint32_t existing_genotype_count;          // only the current number of genotypes in existence
    double maximum_subpopulation_fitness;
    
    // Total number of cell equivalents in population
    uint32_t total_cell_equivalents;    // @JEB: possibly not used
    double current_population_size;    // Current population size in terms of whole cells
    
    // Lineages that divided simultaneously and could all contain a mutation equally well
    vector<uint32_t> just_divided_lineages;

    // New subpopulations that arose this transfer. Allows culling subpopulations did not survive a transfer.
    vector<cSubpopulation>  mutations_since_last_transfer;  
                                                              
    // All of the following should be initialized in the constructor
    int32_t num_completed_transfers;   //Number of transfers that have been completed thus far, can be negative for burn-in
    double average_subpopulation_fitness;
    bool run_end_condition_met;
    
    // @JEB: An double rather than a uint because this can go negative by a few cells
    //       when cells (usually the ancestors) divide simultaneously.
    double num_divisions_until_next_mutation; 
    double num_completed_divisions;

    gsl_rng* rng;
    const double log_2;
    
  public:

    //CONSTRUCTOR  
    cPopulation();
    cPopulation(AnyOption& options, gsl_rng* in_rng, uint32_t in_replicate);
    
    //DESTRUCTOR
    virtual ~cPopulation() { };
    
    void   DisplayParameters();
    void   RunSimulation(bool new_simulation);
    void   DisplaySimulationSummary();

    void   OutputCladeFrequencies(double frequency_threshold = 0);
    void   OutputGenotypeFrequencies(double frequency_threshold = 0);
    void   OutputMullerMatrix(uint32_t frequency_resolution);

    
    /// ---> Need to check output below here
        
    void   PrintUniqueGenotypes(const string& output_folder,
                                vector< vector<uint32_t> > * number_of_unique_genotypes);
    void   PrintExpectationValue(const string& output_folder);
    void   PrintOut_RedWhiteOnly(const string& output_folder,
                                 vector< vector<double> > *red_white_ratios,
                                 uint32_t transfer_interval_to_print);
    void   PrintFrequenciesToScreen(string output_folder);
    void   PrintFrequenciesToScreen_RedWhiteOnly(string output_folder);
    void   PrintSingleFitness(string out_folder);
    void   PrintWinningFitness(string out_folder, uint32_t on_run);
    
  private:
    
    enum e_colors {
      RED=0,
      WHITE=1,
    };

    //!!! Private Simulation Helper Methods
      
    //! Move time forward by this increment, growing all subpopulations
    void   UpdateSubpopulationsForGrowth(double update_time);
    void   UpdateSubpopulationsForGrowthExactWithFractionalCells(double update_time);
    
    //! Calculate the time until the next subpopulation divides (passes a whole number of cells)
    double TimeToNextWholeCell();
    
    //! Calculate an amount of time to divide that won't take us over the time until the next mutation
    double CalculateDivisionsUntilNextBeneficialMutation() 
      { return static_cast<double>(round(gsl_ran_exponential(rng, simulation_parameters.total_mutation_rate_exponential_mean))); }
    void   ProcessCellDivisionTimeStep();
    void   ProcessCellDivisionTimeStepExactWithFractionalCells();
    void   ProcessCellDivisionTimeStepNew();
    
    //! Population transfer methods
    void   TransferResampleDilution();
    void   TransferResampleExactlyOne();
    
    //! Methods for initializing the population
    void   SeedPopulationWithMarkedGenotypes(uint32_t genetic_markers);
    void   SeedPopulationWithOneGenotype();
    
    //! Methods for introducing mutants into the population
    uint32_t  DetermineMutationCategory();
    void      Mutate();
    void      MutateExactWithFractionalCells();
    void      MutateNew();
    
    // General function used whenever a new subpopulation is added
    void   AddSubpopulation(cSubpopulation& subpop);
    
    // Used to remove genotypes that died before a transfer to save memory
    void   CullSubpopulationsThatDidNotEstablish();

    // Methods for recording statistics during a run
    void   RecordStatisticsAtTransfer(); 

    //!!! Utility calculations - mainly used in debug mode
    
    double CalculatePopulationSize();
    double   CalculateMaximumSubpopulationFitness();
    double   CalculateAverageSubpopulationFitness();
    
    double AssignChildFrequency(
                                tree<cGenotype>::sibling_iterator child_node,
                                double parent_low,
                                double parent_high,
                                vector<cFrequencySlice> * child_freqs, 
                                vector<cGenotypeFrequency> &frequencies,
                                int depth = 0
                                );
    
    // Prints out the tree using bracket notation.
    void   PrintTree() { kptree::print_tree_bracketed(genotype_tree); cout << endl; }
    
    //!!! Lookup methods
    
    double GenotypeFrequency(vector<cGenotypeFrequency> &frequencies,
                             uint32_t this_node);
    cSubpopulation* Find_Node_in_Populations_By_NodeID(uint32_t this_node);
    
    
    
    
    //!!! Data Analysis Methods
    
    void   ConvertExternalData(const string &input_file);
    vector<cGenotypeFrequency>::iterator Find_Node_in_Freq(
                                                           vector<cGenotypeFrequency> &frequencies, 
                                                           tree<cGenotype>::sibling_iterator this_node
                                                           );
    
    uint32_t Last_Sweep(float threshold);
    vector<uint32_t> GenotypesFromAncestorToFinalDominant(float threshold);
    vector<uint32_t> GenotypesAboveThreshold(float threshold);
    vector<uint32_t> CladesAboveThreshold(float threshold);

    vector<uint32_t> CurrentUniqueGenotypes();
    
    double CalculateSimilarity(string output_folder);
    double CountMutipleDivergedSubpops();
    void   TimeToSweep(string output_folder);
    
    // Functions we don't really need
    void   PrintFreqsQuick();
    float  Logarithm(float mantissa);
  };

}

#endif
