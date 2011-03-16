#ifndef cSubpopulation_h
#define cSubpopulation_h

#include <gsl/gsl_randist.h>
#include <stdint.h>
#include "cLineageTree.h"

class cSubpopulation {

private:
  long double m_number;
  char m_marker;
	tree<cGenotype>::iterator m_genotype;

public:
  cSubpopulation();
  cSubpopulation(const cSubpopulation& in); // Copy constructor
  virtual ~cSubpopulation() { ; }; 

  const long double GetFitness() { return (*m_genotype).fitness; }
  const long double GetNumber() { return m_number; }
  const char GetMarker() { return m_marker; }
  
	void SetGenotype(tree<cGenotype>::iterator location) { m_genotype = location; }
  void SetNumber(const long double in_number) { m_number = in_number; }
  void SetMarker(const char in_marker) { m_marker = in_marker; } 
  void Transfer(long double success_prob, gsl_rng * randomgenerator);
  long double MutantFitness(long double in_fitness, 
														double in_average_mutation_s, 
														char in_type_of_mutation, 
														gsl_rng * randomgenerator);  

	/*###################################################################################
  @agm Here I added the counter as a crude way to keep track of unique_node_id's.
			 I'm sure there is a better way to do this 
			 Also, notice I pass the entire tree rather than a segment or immediate parent
	     This is likely not ideal or fastest with huge trees.
	 ####################################################################################*/
	
	virtual void NewCreateDescendant(gsl_rng * randomgenerator, 
																	 cSubpopulation &ancestor, 
																	 double averageselectioncoefficient, 
																	 char beneficialmutationdistribution, 
																	 tree<cGenotype> in_tree, 
																	 unsigned int node_id);

};
#endif
