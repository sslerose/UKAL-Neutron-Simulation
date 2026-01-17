# Source Files

## ActionInitialization

Initializes and builds user action classes for each worker thread in multi-threaded mode. This file manages the creation of primary generator, run, event, and stacking actions that control particle generation, data collection, and event processing throughout the simulation.

**ActionInitialization constructor**  
Initializes the action initialization with a pointer to the detector construction. Creates the primary generator messenger for UI and macro file command processing. Initializes the PrimaryGeneratorConfig singleton with default values, permitting config command outputs to the master thread.

*Note: PrimaryGeneratorMessengers are generally created within PrimaryGeneratorAction as thread-local instanes that pull from a UI or macro command fed to the master*

**~ActionInitialization destructor**  
Cleans up the primary generator messenger.

**BuildForMaster**  
Initializes user actions for the master thread in multi-threaded mode. Creates a RunAction without a primary generator (master doesn't generate events) and prints the initial generator configuration.

**Build**  
Creates and registers all user action classes for worker threads. Instantiates PrimaryGeneratorAction for particle generation, RunAction for run-level data management, EventAction for event-level analysis, and StackingAction for particle stack management.

## DetectorConstruction

Defines and constructs the complete detector geometry including the Li-6 glass scintillator, aluminum housing, shielding, and support structures. This mandatory initialization class builds all physical volumes, defines materials with precise compositions, and attaches sensitive detectors for hit recording.

**DetectorConstruction constructor**  
Calculates geometric dimensions and positions for all detector components including the assembly volume, aluminum can, silicon rubber layer, teflon layer, detector glass, and PMT. Initializes materials and creates the detector messenger for runtime geometry modifications.

**~DetectorConstruction destructor**  
Cleans up the rotation matrix and detector messenger.

**Construct**  
Required virtual method that calls ConstructVolumes to build the detector geometry and return the world physical volume.

**DefineMaterials**  
Creates all materials used in the detector assembly using the NIST database and custom formulations. Defines vacuum for the world volume, creates enriched Li-6 isotopes (96% Li-6, 4% Li-7), formulates Li-6 glass scintillator based on Feinberg thesis composition (56% SiO2, 18% Li2O, 18% Al2O3, 4% MgO, 4% Ce2O3), creates aluminum alloy 6061-T6 for the housing, silicon rubber for optical coupling, teflon for light reflection, borosilicate glass for the PMT window, and mu-metal for magnetic shielding.

**ConstructVolumes**  
Builds the complete detector geometry hierarchy. Creates the world box volume, constructs the detector assembly mother volume positioned at the specified distance from the origin, builds the mu-metal holder cylinder, creates the inner assembly volume to contain detector components, constructs the aluminum can with end cap, adds silicon rubber and teflon layers for optical coupling and light reflection, places the Li-6 glass detector cylinder, and adds the borosilicate PMT window. Returns the world physical volume as the root of the geometry tree.

**ConstructSDandField**  
Attaches the sensitive detector to the Li-6 glass logical volume. Creates or retrieves the DetectorSD sensitive detector, registers it with the SD manager, and assigns it to the detector volume to record neutron interactions.

**SetDetectorDistance**  
Modifies the distance from the particle source (origin) to the detector face. Updates the assembly position accounting for the detector angle and notifies the run manager and visualization manager of the geometry change.

**SetDetectorAngle**  
Sets the detector assembly rotation angle about the y-axis. Creates a new rotation matrix, calculates the new position accounting for the angle, updates both rotation and translation of the assembly, and triggers geometry and visualization updates.

**SetSpanningStartAngle**  
Sets the initial spanning angle for cylindrical detector components (allows partial cylinder construction for visualization).

**SetSpanningEndAngle**  
Sets the final spanning angle for cylindrical detector components.

**PrintParameters**  
Outputs the current detector configuration including world size, Li-6 glass detector dimensions and material properties, detector distance from source, and confirms neutron generation at the origin.

## DetectorHit

Represents a single neutron interaction recorded in the Li-6 glass detector during an event. Each hit stores the track ID, particle type, interaction time, position, and process name, enabling reconstruction of neutron capture events and time-of-flight measurements.

**DetectorHitAllocator**  
Thread-local memory allocator for efficient hit object creation in multi-threaded mode.

**Draw**  
Visualizes hits during event display using a color-coded scheme. Draws red filled circles for neutron capture events (nCapture process) and blue circles for all other interactions (elastic scattering, inelastic, etc.), providing visual feedback on detector response and allowing quick identification of capture versus scattering events.

**Print**  
Outputs hit information in human-readable format including track ID, particle type, global time, and process name. Useful for debugging and verification of detector response.

## DetectorMessenger

Provides UI command interface for runtime modification of detector geometry parameters. Creates and manages commands in the /neutronTOF/det/ directory allowing users to interactively adjust detector position, angle, and visualization parameters during simulation initialization or between runs.

**DetectorMessenger constructor**  
Creates the UI command directory structure and initializes all detector control commands. Sets up commands for detector distance (setDetectorDistance), detector angle (setDetectorAngle), spanning angles (setSpanningStartAngle, setSpanningEndAngle), and parameter printing (printParameters). Each command includes guidance text, parameter validation, and availability states.

**~DetectorMessenger destructor**  
Deletes all UI commands and directory objects.

**SetNewValue**  
Processes user commands by calling corresponding detector construction methods. Handles detector distance changes, angle adjustments, spanning angle modifications, and parameter printing requests. Converts UI command values to appropriate units before passing to DetectorConstruction methods.

## DetectorSD

Sensitive detector class that records neutron interactions in the Li-6 glass scintillator volume. This class processes each simulation step within the detector, identifies neutron reactions of interest (elastic scattering, capture, inelastic, and fission), and creates hit objects containing interaction details for subsequent analysis.

**DetectorSD constructor**  
Initializes the sensitive detector with a name and hits collection name, registering the collection for retrieval by EventAction.

**Initialize**  
Creates a new hits collection at the beginning of each event. Initializes the DetectorHitsCollection with the sensitive detector name and collection name, obtains a unique collection ID from the SD manager, and adds the collection to the event.

**ProcessHits**  
Examines each step in the sensitive volume and records neutron interaction processes. Filters for neutron particles, identifies the post-step process, and creates DetectorHit objects for elastic scattering (hadElastic), capture (nCapture), inelastic scattering (neutronInelastic), and fission (nFission) processes. Records track ID, position, time, particle type, and process name in each hit, enabling time-of-flight and capture event analysis.

**EndOfEvent**  
Called at event completion to optionally print hit collection contents. When verbosity is high, outputs the number of hits and detailed information for each hit, useful for debugging detector response and validating neutron interactions.

## EventAction

Processes detector hits at the end of each event to extract physics quantities and fill histograms. This class analyzes the hits collection to identify neutron capture events, calculate time-of-flight, reconstruct neutron energy, and populate histograms and ntuples for subsequent data analysis.

**EventAction constructor**  
Stores a pointer to the primary generator action for accessing generated neutron parameters.

**BeginOfEventAction**  
Called at the start of each event. Currently empty but available for future initialization needs.

**GetHitsCollection**  
Retrieves the hits collection for a given collection ID from the event. Validates that the collection exists and returns an exception if the collection cannot be accessed.

**AnalyzeHits**  
Performs detailed analysis of detector hits for the current event. Retrieves primary neutron energy and angle from the generator, loops through all hits to identify neutron capture events (nCapture process), records the capture time as the time-of-flight, calculates neutron energy from TOF using non-relativistic kinematics (En = 0.5 * mn * (d/t)^2), fills histograms for neutron energy, angle, number of hits, TOF, and TOF-derived energy, and populates the ntuple with event-by-event data including capture flag, TOF, and reconstructed energy.

**EndOfEventAction**  
Called at event completion to retrieve and analyze the hits collection. Obtains the detector hits collection ID (cached after first event), validates the hits collection exists, retrieves the hits collection for the current event, and calls AnalyzeHits to process the data.

**PrintEventStatistics**  
Outputs event summary information including time-of-flight and number of hits. Used for periodic event monitoring during long simulation runs.

## HistoManager

Manages histogram and ntuple creation for data analysis output. This class configures the G4AnalysisManager, defines all histograms for physics quantities (neutron energy, angle, hits, time-of-flight), and creates the ntuple structure for event-by-event ROOT file output.

**HistoManager constructor**  
Calls the Book method to initialize the analysis manager and create all histograms and ntuples.

**Book**  
Configures the analysis manager and creates all data structures for output. Initializes G4AnalysisManager with ROOT file output format, enables ntuple merging for multi-threaded mode, creates histograms for generated neutron energy (0-200 keV, 200 bins), neutron angle (0-90 degrees, 90 bins), number of hits per event (0-10, 10 bins), time-of-flight for capture events (0-300 ns, 300 bins), and TOF-derived neutron energy (0-150 keV, 150 bins). Creates ntuple "DetectorData" with columns for primary neutron energy, angle, capture flag (1=capture, 0=no capture), TOF (-1 if no capture), and TOF energy (-1 if no capture).

## NeutronHPphysics

Physics constructor for high-precision neutron transport using Geant4's ParticleHP models. This class registers neutron interaction processes (elastic scattering, inelastic scattering, capture, and fission) with evaluated nuclear data libraries for accurate simulation of low-energy neutron behavior in the Li-6 detector.

**NeutronHPphysics constructor**  
Initializes the physics constructor with a name and calls DefineCommands to create UI commands for thermal scattering control.

**~NeutronHPphysics destructor**  
Deletes the generic messenger used for UI commands.

**ConstructProcess**  
Registers all neutron physics processes with high-precision models. Removes any existing neutron processes to avoid duplicates, creates hadron elastic process with ParticleHPElastic model and optionally adds thermal scattering model below 4 eV if enabled, registers neutron inelastic process with ParticleHPInelastic model and cross section data, adds neutron capture process with ParticleHPCapture model (includes Li-6(n,α)T reaction), and registers neutron fission process with ParticleHPFission model.

**DefineCommands**  
Creates UI command /neutronTOF/phys/thermalScattering to enable or disable thermal neutron scattering model (S(α,β) treatment for neutrons below 4 eV).

## PhysicsList

Defines the complete set of physics processes active in the simulation. This modular physics list combines electromagnetic physics for charged particle energy loss, decay physics for unstable particles, hadron elastic physics, ion physics, and custom high-precision neutron physics to accurately model neutron generation, transport, capture, and detector response.

**PhysicsList constructor**  
Constructs the complete physics list by registering physics constructors. Sets verbose level to 1 for process information output, defines additional units for surface mass density (mm2/g, um2/mg), registers G4EmStandardPhysics for electromagnetic interactions of tritons and alphas from Li-6 capture, adds G4DecayPhysics for radioactive decay, includes G4HadronElasticPhysicsHP for high-precision elastic scattering, registers G4IonPhysics for alpha and triton nuclear interactions, and adds NeutronHPphysics for high-precision neutron transport including Li-6 capture reaction.

**ConstructParticle**  
Constructs all particle types needed for the simulation including bosons (photons), leptons (electrons, positrons), mesons, baryons (protons, neutrons), ions (alphas, tritons, Li isotopes, Be isotopes), and short-lived particles.

**SetCuts**  
Defines production thresholds for secondary particles. Sets zero production cut for protons to ensure accurate tracking for neutron generation from Li(p,n) reactions, and applies default 1 mm cuts for gammas, electrons, and positrons. Prints the cut values table if verbose output is enabled.

## PrimaryGeneratorAction

Generates primary neutrons for each event using either the SimLiT Li(p,n) neutron source model or a simple particle gun for testing. This class manages neutron production, synchronizes with the shared configuration singleton, and provides neutron energy and angle information to downstream analysis classes.

**PrimaryGeneratorAction constructor**  
Initializes the particle gun for neutron generation, retrieves initial configuration from PrimaryGeneratorConfig singleton, sets default gun energy and direction for simple mode, creates and configures the SimLiT neutron source with beam energy, energy spread, target material, and target thickness, and caches configuration values for efficient change detection.

**~PrimaryGeneratorAction destructor**  
Deletes the particle gun and SimLiT source objects.

**SyncWithConfig**  
Synchronizes local generator parameters with the shared PrimaryGeneratorConfig singleton. Checks each configuration parameter (beam energy, beam sigma, target material, target thickness, gun energy) against cached values and updates only changed parameters to minimize overhead. Handles material name to SimLiT composition enum conversion with error checking.

**GetSimLiTComposition**  
Maps material name strings to SimLiT composition enumeration values. Converts "Li", "LiF", "Li2O", "Li3N", "LiOH", and "LiH" to corresponding SimLiT enum constants, returning -1 for unknown materials.

**GeneratePrimaries**  
Generates primary neutron vertex for each event. Synchronizes with shared configuration to capture UI command changes, checks whether SimLiT or simple gun mode is active, and either generates realistic neutron from Li(p,n) reaction using SimLiT (samples beam energy from Gaussian, determines if proton reacts in target, calculates neutron energy and angle from reaction kinematics, generates random azimuthal angle for 3D direction) or generates simple gun neutron along z-axis for testing. Stores generated neutron energy and angle for access by EventAction analysis.

## PrimaryGeneratorConfig

Thread-safe singleton that stores and shares primary generator configuration parameters across all threads. This class enables UI commands to modify neutron source settings (SimLiT beam parameters, gun energy, output filename) before and during runs in multi-threaded mode by using mutex locks to prevent race conditions.

**Instance**  
Returns the singleton instance using double-checked locking pattern with mutex for thread safety.

**PrimaryGeneratorConfig constructor**  
Initializes default configuration values including SimLiT disabled, beam energy 1912 keV, beam sigma 10 keV, LiF target material, target thickness 10 micrometers, gun energy 50 keV, and empty output filename.

**SetUseSimLiT / GetUseSimLiT**  
Thread-safe setter and getter for SimLiT enable/disable flag with automatic console output.

**SetBeamEnergy / GetBeamEnergy**  
Thread-safe access to proton beam energy in keV for SimLiT source.

**SetBeamSigma / GetBeamSigma**  
Thread-safe access to proton beam energy spread (1-sigma) in keV.

**SetTargetMaterial / GetTargetMaterial**  
Thread-safe access to lithium target material name string.

**SetTargetThickness / GetTargetThickness**  
Thread-safe access to target thickness in micrometers.

**SetGunEnergy / GetGunEnergy**  
Thread-safe access to particle gun energy when SimLiT is disabled.

**PrintParameters**  
Outputs current configuration to console including source mode (SimLiT or gun), beam parameters or gun energy, target properties, and reaction threshold energy.

**SetOutputFileName / GetOutputFileName / HasOutputFileName / ClearOutputFileName**  
Thread-safe management of the ROOT output filename, used by master thread to coordinate filename generation with worker threads for proper ntuple merging.

## PrimaryGeneratorMessenger

Creates UI commands for interactive control of the neutron source configuration. This messenger class defines commands in the /neutronTOF/gun/ directory that modify the PrimaryGeneratorConfig singleton, allowing users to switch between SimLiT and gun modes, adjust beam parameters, change target properties, and control output settings.

**PrimaryGeneratorMessenger constructor**  
Creates the /neutronTOF/gun/ UI directory and initializes all neutron source commands including useSimLiT (enable/disable SimLiT source), beamEnergy (proton beam energy in keV with threshold validation), beamSigma (energy spread in keV), targetMaterial (Li compound with candidates list), targetThickness (thickness in micrometers), energy (gun energy with units), and printParameters (display current configuration). Each command includes detailed guidance, parameter validation, and available states.

**~PrimaryGeneratorMessenger destructor**  
Deletes all UI commands and the directory.

**SetNewValue**  
Processes UI commands by updating the PrimaryGeneratorConfig singleton. Retrieves the config instance and calls the appropriate setter method for each command, converting units where necessary (gun energy command includes unit handling). All commands modify the shared configuration that is read by PrimaryGeneratorAction during event generation.

## Run

Custom run class that accumulates neutron transport statistics across events. This class extends G4Run to collect data on particle creation, process frequencies, track lengths, and collision rates for analysis of neutron behavior in the detector material, primarily used for physics validation and cross section verification.

**Run constructor**  
Stores pointer to detector construction for accessing detector material properties.

**SetPrimary**  
Records the primary particle type and kinetic energy for run summary output.

**CountProcesses**  
Increments the counter for a given physics process name, building a map of process frequencies throughout the run.

**ParticleCount**  
Records secondary particle creation by updating the particle data map. For each particle species, tracks the count, cumulative energy (for mean calculation), minimum energy, and maximum energy.

**SumTrackLength**  
Accumulates track statistics separating high-energy (E>1 eV) and low-energy (E<1 eV) regimes. Sums the number of steps, total track length, and time-of-flight for each energy range.

**Merge**  
Combines data from worker thread runs in multi-threaded mode. Merges process counters, particle data maps, and track length statistics from local run into master run, properly handling energy min/max comparisons and cumulative sums.

**EndOfRun**  
Prints comprehensive run summary including run conditions (particle type, energy, detector material, density), process call frequencies with identification of surviving particles, neutron track statistics split by energy regime (collision counts, track lengths, time-of-flight), and list of all generated particles with their mean, minimum, and maximum energies.

## RunAction

Controls run-level operations including analysis manager configuration, output file creation, and run summary generation. This class manages histogram output, generates descriptive filenames based on source configuration, handles multi-threaded filename synchronization for ntuple merging, and prints analysis statistics at run completion.

**RunAction constructor**  
Stores pointers to detector construction and primary generator action, and creates the HistoManager to book histograms and ntuples.

**~RunAction destructor**  
Deletes the HistoManager.

**GenerateFileName**  
Creates descriptive output filename from current configuration. Reads source type (SimLiT or Gun), beam/gun energy, and detector angle from configuration singletons, and formats filename as "nDet_<source>_<energy>keV_<angle>deg" with integer and decimal parts for precise identification of simulation conditions.

**GenerateRun**  
Factory method that creates custom Run object for accumulating neutron transport statistics.

**BeginOfRunAction**  
Initializes the run by showing random number engine status, storing primary particle information in the Run object, and configuring analysis manager output. Master thread generates the filename and stores it in PrimaryGeneratorConfig, worker threads wait for filename availability (spin-wait with 1ms delay, 1 second timeout), all threads set the same filename and open the output file to ensure proper ntuple merging in multi-threaded mode.

**EndOfRunAction**  
Finalizes the run by calling Run::EndOfRun to print neutron transport statistics, displaying analysis summary including SimLiT neutron source statistics (mean energy and angle with RMS), detector response (events with energy deposition, mean deposited energy), and TOF measurement statistics (capture event count, mean TOF, TOF RMS). Writes histograms and ntuple to ROOT file, closes the file, clears the stored filename for next run, and shows final random number engine status.

## SimLiT

Simulates neutron production from the 7Li(p,n)7Be and 7Li(p,n)7Be* reactions using experimental cross section data and reaction kinematics. This external library by Moshe Friedman calculates realistic neutron energy and angular distributions from a proton beam on lithium targets, accounting for beam energy spread, target thickness, energy loss, and reaction probabilities.

**SimLiT constructor**  
Initializes the neutron source with beam energy and energy spread. Calls Find_Parameters to determine cross section fit parameters, sets default beam radius (2 mm), proton counter to zero, target composition (LiF), and target thickness (20 micrometers), then constructs lookup tables for energy loss (ConstructDeltaEVector) and reaction cumulative distribution functions (ConstructCDFVectors).

**~SimLiT destructor**  
Prints debug statistics including overflow rate and tries rate when the source has been used.

**Find_Parameters**  
Calculates cross section normalization parameter A by matching analytical fit to first measured data point at 1886.7 keV. Uses threshold law fit C*sqrt(1-Eth/E)/(1+C*sqrt(1-Eth/E))^2 below 1886.7 keV and interpolates tabulated data above this energy.

**GenerateNeutron**  
Main neutron generation method that returns energy (keV) and angle (radians) by reference. Checks beam energy exceeds reaction threshold, generates protons until one reacts in the target, determines reaction parameters (energy, angle in CM frame), transforms kinematics from center-of-mass to lab frame, and calculates final neutron energy and angle.

**SetBeamEnergy / GetBeamEnergy**  
Sets or retrieves proton beam mean energy in keV, reconstructing lookup tables when changed.

**SetBeamSigma / GetBeamSigma**  
Sets or retrieves beam energy spread (1-sigma) in keV, reconstructing lookup tables when changed.

**SetBeamRadius / GetBeamRadius**  
Sets or retrieves Gaussian beam radius in cm for spatial distribution of reaction position.

**GetNeutronVelocity**  
Returns neutron velocity vector in cm/s after transforming to lab frame.

**GetNeutronPosition**  
Returns transverse position (x,y) in cm where proton reacted in target.

**SetProtonCounter / GetProtonCounter**  
Tracks number of protons required to produce one neutron for yield calculations.

**SetComposition / GetComposition**  
Sets or retrieves target composition (Li, LiF, Li2O, Li3N, LiOH, LiH) affecting density and stopping power.

**SetTargetThickness / GetTargetThickness**  
Sets or retrieves target thickness in cm, reconstructing lookup tables when changed.

**GetNormalOrStar**  
Returns boolean indicating whether last neutron came from ground state (false) or excited state (true) reaction.

**GetGauss**  
Generates normally distributed random numbers using Box-Muller method with specified mean and sigma.

**GenerateProton**  
Samples proton energy from Gaussian beam distribution, generates random transverse position from 2D Gaussian with beam radius, and increments proton counter.

**GetDistribution**  
Calculates differential cross section dσ/dΩ (mb/sr) times stopping power factor at given proton energy and CM angle. Uses threshold fit below 1886.7 keV, interpolates Legendre polynomial coefficients from tabulated data above threshold, evaluates angular distribution σ(θ) = σ_0(a_0 + a_1*P_1 + a_2*P_2 + a_3*P_3), includes 7Be* excited state contribution above 2373 keV, and multiplies by (ρN_A/M_A)*(1/dE/dx) to get reaction probability per energy interval.

**ConstructCDFVectors**  
Pre-calculates cumulative distribution function tables for reaction probability versus proton energy. Integrates GetDistribution over energy from threshold to beam energy +3σ, accounting for energy loss in target, stores both high-boundary and low-boundary CDFs for proper handling of finite target thickness, calculates normalization constant for total reaction probability, and enables fast lookup during neutron generation.

**GetCDF**  
Retrieves CDF value for given proton energy by interpolating pre-calculated vectors.

**ProtonReact**  
Determines if a proton will react in the target using Monte Carlo sampling. Calculates probability for ground state and excited state reactions from CDF differences, generates random number and compares to probabilities, sets flag indicating which reaction occurred, and returns true if reaction happens.

**CalculateDeltaECm**  
Computes available kinetic energy in center-of-mass frame. Transforms proton energy to CM frame, subtracts Q-value (1644.2 keV for ground state, 2075.0 keV for excited state), and returns energy available for products.

**GenerateNeutronVelocity**  
Generates neutron velocity vector in CM frame given CM polar angle. Samples azimuthal angle uniformly, calculates neutron energy in CM from kinematics, and constructs 3D velocity vector in spherical coordinates.

**DetermineReactionParameters**  
Monte Carlo sampling of reaction energy and angle in CM frame. Calculates energy loss in target, samples proton energy uniformly in loss interval, samples CM angle using rejection method with angular distribution as probability density, determines which state (ground or excited) reacted, stores reaction energy, and generates neutron velocity in CM frame.

**TransformCMToLab**  
Boosts neutron velocity from center-of-mass frame to laboratory frame by adding CM velocity relative to lab.

**ClaculateNeutronEnergy**  
Calculates neutron kinetic energy from velocity magnitude using non-relativistic formula E = 0.5*m*v^2.

**CalculateNeutronTheta**  
Computes polar angle of neutron in lab frame from velocity components using θ = arccos(v_z/|v|).

**PrintCDFvectors**  
Debug output of CDF lookup tables showing energy, energy loss, and CDF values.

**getStoppingPower**  
Evaluates proton stopping power dE/dx (keV/μm) in target using cubic polynomial fit for the specified composition.

**calculateDeltaE**  
Numerically integrates stopping power to find total energy loss as proton traverses target thickness using small step size (1 nm).

**ConstructDeltaEVector**  
Pre-calculates energy loss lookup table by calling calculateDeltaE for proton energies spanning beam energy ±5σ range.

**getDeltaE**  
Retrieves energy loss for given proton energy from pre-calculated vector via interpolation.

**getDeltaEReversed**  
Inverse function that finds initial proton energy yielding specified energy loss, used for proper CDF integration. Implements bisection method to solve E_initial  ΔE(E_initial) = E_final.

## StackingAction

Controls which particles are tracked during event processing. This stacking action implements a particle filter that retains only the primary neutron for tracking while killing all secondary particles, significantly reducing computation time by eliminating tracking of reaction products that don't contribute to time-of-flight measurements.

**ClassifyNewTrack**  
Determines the fate of each newly created track. Returns fUrgent for the primary particle (TrackID == 1) to ensure it is tracked immediately, and returns fKill for all secondary particles (alphas, tritons, gammas, etc.) to prevent their tracking. This optimization is valid because detector response (capture time) depends only on primary neutron interaction, not secondary particle tracking.