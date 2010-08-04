#include "cSubpopulation.h"

/* */
cSubpopulation::cSubpopulation()
{
  m_fitness = 0;
  m_number = 0;
  m_marker = 0;
}

/* Copy constructor */
cSubpopulation::cSubpopulation(const cSubpopulation& in)
{
  m_fitness = in.m_fitness;
  m_number = in.m_number;
  m_marker = in.m_marker;
}

long double cSubpopulation::MutantFitness(long double in_fitness, double in_average_mutation_s, char in_type_of_mutation, gsl_rng * randomgenerator)
{
	if(in_type_of_mutation=='e')
	{
		return gsl_ran_exponential(randomgenerator,in_average_mutation_s) + in_fitness;
		
	}
	
	if (in_type_of_mutation=='u')
	{
		return in_fitness + in_average_mutation_s;
	}

	return 0;

}
cSubpopulation& cSubpopulation::CreateDescendant(gsl_rng * randomgenerator)
{
  //clone ourself
  cSubpopulation& new_sp = *(new cSubpopulation(*this));
  
  //and there is only one new one
  new_sp.SetNumber(1);
  //taken from the ancestor
  SetNumber(GetNumber()-1);

   //and give new fitness
  new_sp.SetFitness(MutantFitness(GetFitness(), .05, 'e', randomgenerator));

  return new_sp;
}

void cSubpopulation::Transfer(long double success_prob, gsl_rng * randomgenerator)
{
	SetNumber(gsl_ran_binomial(randomgenerator, success_prob, uint64_t(GetNumber())));
}

