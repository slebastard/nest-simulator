/*
 *  iaf_psc_exp_ps_lossless.h
 *
 *  This file is part of NEST.
 *
 *  Copyright (C) 2004 The NEST Initiative
 *
 *  NEST is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  NEST is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with NEST.  If not, see <http://www.gnu.org/licenses/>.
 *
 */


#ifndef IAF_PSC_EXP_PS_LOSSLESS_H
#define IAF_PSC_EXP_PS_LOSSLESS_H

#include "config.h"

#include "archiving_node.h"
#include "nest_types.h"
#include "event.h"
#include "ring_buffer.h"
#include "slice_ring_buffer.h"
#include "connection.h"
#include "universal_data_logger.h"
#include "stopwatch.h"
#include "arraydatum.h"

#include <vector>

/*Begin Documentation
Name: iaf_psc_exp_ps_lossless - Leaky integrate-and-fire neuron
with exponential postsynaptic currents; precise implementation;
predicts exact number of spikes by applying state space analysis

Description:
iaf_psc_exp_ps_lossless is the precise state space implementation of the leaky
integrate-and-fire model neuron with exponential postsynaptic currents
that uses time reversal to detect spikes [1]. This is the most exact 
implementation available.

Time-reversed state space analysis provides a general method to solve the
threshold-detection problem for an integrable, affine or linear time
evolution. This method is based on the idea of propagating the threshold
backwards in time, and see whether it meets the initial state, rather
than propagating the initial state forward in time and see whether it
meets the threshold. 

Parameters: 
  The following parameters can be set in the status dictionary.
  E_L           double - Resting membrane potential in mV.
  C_m           double - Specific capacitance of the membrane in pF/mum^2.
  tau_m         double - Membrane time constant in ms.
  tau_syn_ex    double - Excitatory synaptic time constant in ms.
  tau_syn_in    double - Inhibitory synaptic time constant in ms.
  t_ref         double - Duration of refractory period in ms.
  V_th          double - Spike threshold in mV.
  I_e           double - Constant input current in pA.
  V_min         double - Absolute lower value for the membrane potential.
  V_reset       double - Reset value for the membrane potential.
 
References:
[1] J.Krishnan, P.G.L.Porta Mana, M.Helias, M.Diesmann, E.Di.Napoli
(2017) Perfect spike detection via time reversal, arXiv:1706.05702, 
submitted to Front. Neuroinformatics.

Author: Jeyashree Krishnan

Sends: SpikeEvent

Receives: SpikeEvent, CurrentEvent, DataLoggingRequest

SeeAlso: iaf_psc_exp_ps
*/ 

namespace nest
{
  /**
   * Leaky iaf neuron, exponential PSC synapses, canonical implementation.
   * @note Inherit privately from Node, so no classes can be derived
   * from this one.
   * @todo Implement current input in consistent way.
   */
  class iaf_psc_exp_ps_lossless : public Node
  {
    
    class Network;
    
  public:
    
    /** Basic constructor.
        This constructor should only be used by GenericModel to create 
        model prototype instances.
    */
    iaf_psc_exp_ps_lossless();
    
    /** Copy constructor.
	GenericModel::allocate_() uses the copy constructor to clone
	actual model instances from the prototype instance. 
	
	@note The copy constructor MUST NOT be used to create nodes based
	on nodes that have been placed in the network.
    */ 
    iaf_psc_exp_ps_lossless(const iaf_psc_exp_ps_lossless &);
    
    /**
     * Import sets of overloaded virtual functions.
     * We need to explicitly include sets of overloaded
     * virtual functions into the current scope.
     * According to the SUN C++ FAQ, this is the correct
     * way of doing things, although all other compilers
     * happily live without.
     */
    using Node::handles_test_event;
    using Node::handle;
    
    port send_test_event( Node&, rport, synindex, bool );
    
    void handle(SpikeEvent &);
    void handle(CurrentEvent &);
    void handle(DataLoggingRequest &);
    
    port handles_test_event(SpikeEvent &, port);
    port handles_test_event(CurrentEvent &, port);
    port handles_test_event(DataLoggingRequest &, port);
    
    bool is_off_grid() const  // uses off_grid events
    {
      return true;
    }
    
    void get_status(DictionaryDatum &) const;
    void set_status(const DictionaryDatum &);
    
  private:

    /** @name Interface functions
     * @note These functions are private, so that they can be accessed
     * only through a Node*. 
     */
    //@{
    void init_node_(const Node & proto);
    void init_state_(const Node & proto);
    void init_buffers_();
    void calibrate();

    /**
     * Time Evolution Operator.
     *
     * update() promotes the state of the neuron from origin+from to origin+to.
     * It does so in steps of the resolution h.  Within each step, time is
     * advanced from event to event, as retrieved from the spike queue.  
     *
     * Return from refractoriness is handled as a special event in the
     * queue, which is marked by a weight that is GSL_NAN.  This greatly simplifies
     * the code.
     *
     * For steps, during which no events occur, the precomputed propagator matrix
     * is used.  For other steps, the propagator matrix is computed as needed.
     *
     * While the neuron is refractory, membrane potential (y2_) is
     * clamped to U_reset_.
     */
    void update(Time const & origin, const long from, const long to);
    //@}
    
    // The next two classes need to be friends to access the State_ class/member
    friend class RecordablesMap<iaf_psc_exp_ps_lossless>;
    friend class UniversalDataLogger<iaf_psc_exp_ps_lossless>;
    
    void set_spiketime(Time const &);
    
    /**
     * Propagate neuron state.
     * Propagate the neuron's state by dt.
     * @param dt Interval over which to propagate
     */
    void propagate_(const double dt);
    
    /**
     * Emit a single spike caused by DC current in absence of spike input.
     * Emits a single spike and reset neuron given that the membrane
     * potential was below threshold at the beginning of a mini-timestep
     * and above afterwards.
     *
     * @param origin  Time stamp at beginning of slice
     * @param lag     Time step within slice
     * @param t0      Beginning of mini-timestep
     * @param dt      Duration of mini-timestep
     */
    void emit_spike_(const Time & origin, const long lag, 
		     const double t0, const double dt);
    
    /**
     * Emit a single spike at a precisely given time.
     *
     * @param origin        Time stamp at beginning of slice
     * @param lag           Time step within slice
     * @param spike_offset  Time offset for spike
     */
    void emit_instant_spike_(const Time & origin, const long lag, 
			     const double spike_offset);
    
    /**
     * Localize threshold crossing by bisectioning.
     * @param   double length of interval since previous event
     * @returns time from previous event to threshold crossing
     */
    double bisectioning_(const double dt) const;

    bool is_spike_(const double);

    // ---------------------------------------------------------------- 

    /** 
     * Independent parameters of the model. 
     */
    struct Parameters_
    {
      /** Membrane time constant in ms. */
      double tau_m_; 
      
      /** Time constant of exc. synaptic current in ms. */
      double tau_ex_;
      
      /** Time constant of inh. synaptic current in ms. */
      double tau_in_;
      
      /** Membrane capacitance in pF. */
      double c_m_;
      
      /** Refractory period in ms. */
      double t_ref_;
      
      /** Resting potential in mV. */
      double E_L_;
      
      /** External DC current [pA] */
      double I_e_;
      
      /** Threshold, RELATIVE TO RESTING POTENTAIL(!).
          I.e. the real threshold is U_th_ + E_L_. */
      double U_th_;
      
      /** Lower bound, RELATIVE TO RESTING POTENTAIL(!).
          I.e. the real lower bound is U_min_+E_L_. */
      double U_min_;
      
      /** Reset potential. 
	  At threshold crossing, the membrane potential is reset to this value. 
	  Relative to resting potential. */
      double U_reset_;
      
      Parameters_();  //!< Sets default parameter values
      // void calc_const_is_spike_();

      void get(DictionaryDatum &) const;  //!< Store current values in dictionary
      double set(const DictionaryDatum &);  //!< Set values from dicitonary
    
    };
    
    // ---------------------------------------------------------------- 

    /**
     * State variables of the model.
     */
    struct State_
    {
      double y0_;  //!< External input current
      double I_syn_ex_;  //!< Exc. exponetial current
      double I_syn_in_;  //!< Inh. exponetial current
      double y2_;  //!< Membrane potential (relative to resting potential)
      
      bool is_refractory_;  //!< True while refractory
      long   last_spike_step_;  //!< Time stamp of most recent spike
      double last_spike_offset_;  //!< Offset of most recent spike

      State_();  //!< Default initialization
      
      void get(DictionaryDatum &, const Parameters_ &) const;
      void set(const DictionaryDatum &, const Parameters_ &, double delta_EL);
    };
    
    // ---------------------------------------------------------------- 

    /**
     * Buffers of the model.
     */
    struct Buffers_
    {
      Buffers_(iaf_psc_exp_ps_lossless &);
      Buffers_(const Buffers_ &, iaf_psc_exp_ps_lossless &);

      /**
       * Queue for incoming events.
       * @note Handles also pseudo-events marking return from refractoriness.
       */
      SliceRingBuffer events_;
      RingBuffer currents_; 
      
      //! Logger for all analog data
      UniversalDataLogger<iaf_psc_exp_ps_lossless> logger_;
    };

    // ---------------------------------------------------------------- 

    /**
     * Internal variables of the model.
     */
    struct Variables_
    { 
      double h_ms_;              //!< Time resolution [ms]
      long   refractory_steps_;  //!< Refractory time in steps
      double expm1_tau_m_;       //!< exp(-h/tau_m) - 1
      double expm1_tau_ex_;      //!< exp(-h/tau_ex) - 1
      double expm1_tau_in_;      //!< exp(-h/tau_in) - 1
      double P20_;               //!< Progagator matrix element, 2nd row
      double P21_in_;            //!< Progagator matrix element, 2nd row
      double P21_ex_;            //!< Progagator matrix element, 2nd row
      double y0_before_;         //!< y0_ at beginning of ministep
      double I_syn_ex_before_;      //!< y1_ at beginning of ministep
      double I_syn_in_before_;      //!< y1_ at beginning of ministep
      double y2_before_;         //!< y2_ at beginning of ministep
      double bisection_step;     // if missed spike is detected, calculate time to emit spike

      // The following are variables that are precomputed for the _is_spike function. 
      // a1_, a2_, a3_, a4_ are constants that appear in the inequality V < g(h, I_e).
      double a1_;
      double a2_;
      double a3_;
      double a4_;

      // b1_, b2_, b3_, b4_, in the line corresponding to the final timestep, V < f(h, I).
      double b1_;
      double b2_;
      double b3_;
      double b4_;

      //c1_, c2_, c3_, c4_, c5_, c6_ in the envelope, V < b(I_e).
      double c1_;
      double c2_;
      double c3_;
      double c4_;
      double c5_;
      double c6_;
	   };
    
    // Access functions for UniversalDataLogger -------------------------------
    
    //! Read out the real membrane potential
    double get_V_m_() const { return S_.y2_ + P_.E_L_; }
    double get_I_syn_() const { return S_.I_syn_ex_ + S_.I_syn_in_; }
    double get_I_syn_ex_() const { return S_.I_syn_ex_; }
    double get_I_syn_in_() const { return S_.I_syn_in_; }

    
    // ---------------------------------------------------------------- 
    
    /**
     * @defgroup iaf_psc_exp_ps_lossless_data
     * Instances of private data structures for the different types
     * of data pertaining to the model.
     * @note The order of definitions is important for speed.
     * @{
     */   
    Parameters_ P_;
    State_      S_;
    Variables_  V_;
    Buffers_    B_;
    /** @} */
    
    //! Mapping of recordables names to access functions
    static RecordablesMap<iaf_psc_exp_ps_lossless> recordablesMap_;
  };
  
inline
port iaf_psc_exp_ps_lossless::send_test_event( Node& target, rport receptor_type, synindex, bool )
{
  SpikeEvent e;
  
  e.set_sender(*this);
  //c.check_event(e);
  return target.handles_test_event( e, receptor_type );
}

inline
port iaf_psc_exp_ps_lossless::handles_test_event(SpikeEvent &, port receptor_type)
{
  if (receptor_type != 0)
    throw UnknownReceptorType(receptor_type, get_name());
  return 0;
}

inline
port iaf_psc_exp_ps_lossless::handles_test_event(CurrentEvent &, port receptor_type)
{
  if (receptor_type != 0)
    throw UnknownReceptorType(receptor_type, get_name());
  return 0;
}

inline
port iaf_psc_exp_ps_lossless::handles_test_event(DataLoggingRequest & dlr, 
				    port receptor_type)
{
  if (receptor_type != 0)
    throw UnknownReceptorType(receptor_type, get_name());
  return B_.logger_.connect_logging_device(dlr, recordablesMap_);
}

inline
void iaf_psc_exp_ps_lossless::get_status(DictionaryDatum & d) const
{
  P_.get(d);
  S_.get(d, P_);
  (*d)[names::recordables] = recordablesMap_.get_list();

}

inline
void iaf_psc_exp_ps_lossless::set_status(const DictionaryDatum & d)
{
  Parameters_ ptmp = P_;  // temporary copy in case of errors
  double delta_EL = ptmp.set(d);            // throws if BadProperty
  State_ stmp = S_;       // temporary copy in case of errors
  stmp.set(d, ptmp, delta_EL);      // throws if BadProperty

  // if we get here, temporaries contain consistent set of properties
  P_ = ptmp;
  S_ = stmp;
}


  
} // namespace

#endif // IAF_PSC_EXP_PS_LOSSLESS_H


