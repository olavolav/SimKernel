/***********************************************************************
   sim_control.h   -   Simulation Flow Control

   Christoph Kirst
   christoph@nld.ds.mpg.de 
   Max Planck Institue for Dynamics and Self-Organisation
   HU Berlin, BCCN Göttingen & Berlin (2008)
************************************************************************/
#ifndef SIM_CONTROL_H
#define SIM_CONTROL_H

#include <sstream>

#include "expression_parser.h"
#include "sim_io_manager.h"
#include "sim.h"


#ifndef SIM_PRE
#define SIM_PRE "Sim: "
#endif


template<typename KernelT>
class SimControl
{
private:
    // io
   SimIOManager      io;

   // parameter
   Sim               sim;

   // parser
   ExprParser        parser;

public:

   SimControl() {};

   int simulate(int argc, char* argv[])
   {

      io.init(true, SIM_PRE, true, false, "sim.log");
      sim.init_io(&io);

      if (argc<2) 
      {
         io.message("No input file specified!", Exit);
         io.close();
         return int(Exit);
      };

      std::string err;
      if (!parser.parse_file(argv[1], err))
      {
         io.message(err);
         io.message("Simulation initialization failed!", Exit);
         io.close();
         return int(Exit);
      };

      if (!sim.init(parser.result, err))
      {
         io.message(err);
         io.message("Simulation initialization failed!", Exit);
         io.close();
         return int(Exit);
      };

      io.set_iterations(sim.n_iterations());

      SimSignal reterr = Success;

      while (sim.next_iteration())
      {
         try {
            std::stringstream str;
            str << "Starting Simulation iteration: " << (sim.iteration());
            str << "/" << sim.n_iterations() << std::endl;
            io.message(str.str());
   
            KernelT kernel;
   
            kernel.initialize(sim);
            kernel.execute(sim);
            kernel.finalize(sim);

            std::stringstream str2;
            str2 << "Simulation iteration: " << (sim.iteration());
            str2 << "/" << sim.n_iterations() << " done!" << std::endl;
            io.message(str2.str());
         }
         catch (const SimSignal& e)
         {
            SIM_DEBUG("catch sig=", e)
            switch (e)
            {
               case Abort:
                  io.message("Simulation run aborted due to error!");
                  reterr = Abort;
                  break;
               case Exit:
               default:
                  io.message("Simulation aborted due to fatal error!");
                  io.summary();
                  io.close();
                  return int(Exit);
            };
         };
      }; // while

      io.message("Simulation done!\n");
      io.error_summary();
      io.message("Bye!");
      io.close();

      return int(reterr);
   };

   int simulate_online(int argc, char* argv[])
   {
      int in = 1;

      io.init(true, SIM_PRE, true, false, "sim.log");
      sim.init_io(&io);

      std::string err;
      ExprPtrT g = ExprPtrT(new ExprGlobal());
      if (!sim.init(g, err))
      {
         io.message(err);
         io.message("Sim initialization failed!", Exit);
         io.close();
         return int(Exit);
      };

      std::string cmd;
      err = "";
      cout << "In["<<in<<"]:";
      cin >> cmd;
      while (cmd != "Quit[]" && cmd != "q")
      {
         if (!parser.parse(cmd.c_str(), err))
         {
            io.message("Cannot parse command: " + cmd);
            io.message(err);
            err = "";
         } else {
            ExprPtrT res, resprt;
//cout << parser.result << std::endl;
            try {
               resprt = ExprPtrT(new ExprPrint(parser.result->arg[0]));
               cout << "Out["<<in<<"]:";
               res = resprt->evaluate(&sim.scope);
            }
            catch (const SimSignal& e)
            {
               io.message("evaluation error!");
               switch (e)
               {
                  case Abort:
                  case Exit:
                     cmd = "q"; break;
                  default:
                     cmd = ""; break;
               }
            }
            cout << "Result["<<in<<"]:" << res << std::endl;
         }
         in++;
         cout << "In["<<in<<"]:";
         cin >> cmd;
      }; // while 

      io.message("Bye!");
      io.close();

      return 0;
   };


};

#endif
