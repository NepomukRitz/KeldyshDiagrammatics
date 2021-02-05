from transformations import Trafo, generate_full_group, CompositeTrafo
from diagram import generate_diagrams, establish_dictionary, ParityTrafo, causality_enforcer
import global_parameters as gp

# Press the green button in the gutter to run the script.
if __name__ == '__main__':

    # Create T transformation objects
    t0 = Trafo(0)
    t1 = Trafo(1)       # Exchange of incoming legs
    t2 = Trafo(2)       # Exchange of outgoing legs
    t3 = Trafo(3)       # Exchange of incoming and outgoing legs
    tc = Trafo(4)       # Complex conjugation
    ts = Trafo(5)       # Spin flip
    tph = Trafo(6)      # Particle-hole symmetry

    # Define the set of transformations to generate the group
    if gp.matsubara:
        tr = Trafo(7)  # Hamiltonian is real function of creation and annihilation operators
        generators = [t0, t1, t2, t3, tc, tr]  # , tph]
    else:
        generators = [t0, t1, t2, t3, tc]  # , tph]

    # Generate full group of T transformations and the full set of diagrams it will operate on
    symmetry_group = generate_full_group(generators)
    all_diagrams = generate_diagrams()

    # Generate a dictionary of all diagrams to store information on dependencies
    dependencies = establish_dictionary(all_diagrams)
    num_indep = 0 

    # Erathostenes' Sieve-based labeling of dependencies
    for diagram in all_diagrams:
        if not dependencies[diagram.generate_key()]:
            num_indep += 1
            # Get first diagrams related through P-trafo's, since they're exactly equal
            parity_group = diagram.parity_group()

            parity_diagrams = [diagram]
            for p_trafo in parity_group:
                parity_diagrams.append(p_trafo.T(diagram))

            eq_to_zero = causality_enforcer(parity_diagrams)

            if not eq_to_zero:
                for p_trafo in parity_group:
                    dependencies[p_trafo.T(diagram).generate_key()] = [p_trafo, diagram.generate_key()]

            for p_diagram in parity_diagrams:
                # Act on current diagram with whole T transformation group
                for trafo in symmetry_group:
                    # Assert that spin sector is allowed
                    if trafo.T(p_diagram).get_spin_indices() not in gp.spin_combinations:
                        # False evaluation implies need for spin flip.
                        trafo = CompositeTrafo(trafo, ts)

                    transformed = trafo.T(p_diagram)

                    # Check if transformed diagram has already been visited
                    if not dependencies[transformed.generate_key()]:
                        # If not, mark dependency with both needed transformation and original diagram
                        if eq_to_zero:
                            dependencies[transformed.generate_key()] = [0]
                        else:
                            dependencies[transformed.generate_key()] = [trafo, diagram.generate_key()]
                        continue

                    # If it has been visited but with a more complex trafo, favor a simpler trafo
                    if type(dependencies[transformed.generate_key()][0]) == CompositeTrafo and type(trafo) == Trafo:
                        dependencies[transformed.generate_key()] = [trafo, diagram.generate_key()]

    # Print out dependencies dictionary
    ii = 0
    for key, value in dependencies.items():
        ii += 1
        if len(value) == 1:
            print(f"{ii:04}: {key} = 0")
            continue

        if type(value[0]) == ParityTrafo:
            print(f"{ii:04}: {key} = P {value[1]}")

        else:
            if value[0] == t0:
                print(f"{ii:04}: {key} is independent!")
                continue
            else:
                print(f"{ii:04}: {key} = {value[0]} {value[1]}")

    # Print number of independent components:
    print(f"There are {num_indep} independent diagrammatic contributions out of a total of {len(all_diagrams)}.")
