import re
import csv
import glob
import os
import fnmatch

def parseGaussianOutput( outputFile, name ):

    res = {
            'name': name, 'v':'1', 'energy':'', 'electronic_state': '', 'eigenvalues_occ': [], 'eigenvalues_virt': [],
            'raman_activ': [], 'frequencies': [], 'ir_values': [], 'dipole_x':'', 'dipole_y':'', 'dipole_z':'',
            'quadrupole_xx': '', 'quadrupole_yy': '', 'quadrupole_zz': '', 
            'quadrupole_xy': '', 'quadrupole_xz': '', 'quadrupole_yz': '',
            'polarizability_axx': '', 'polarizability_axy': '', 'polarizability_ayy': '', 'polarizability_axz': '', 'polarizability_ayz': '', 'polarizability_azz': '',
            'ehomo': '', 'elumo': '', 'temperature': '', 'pressure': '',
            'qC1' : '',
            'thermal_total_e': '', 'thermal_total_s': '', 'thermal_total_cv': '', 
            'principal_axes_eigenvalues': '', 'principal_axes_x': '', 'principal_axes_y': '', 'principal_axes_z': '',
            'sum_zero_energies': '', 'sum_thermal_energies': '', 'sum_thermal_enthalpies': '', 'sum_thermal_free_energies': '',
            'tube_length' : '', 'tube_chirality1' : '', 'tube_chirality2' : ''    
            }
    if outputFile is None:
        return res
    
    lines = outputFile.read( ).split( '\n' )

    def scan( s, *fmt ):
        return tuple( f( v ) for f, v in zip( fmt, s.split() ) )
    
    #name format 'tube_l5_ch4x6_'
    if 'tube' in name:
        res['tube_length'] = re.search('_l(\d+)_', name).group(1)
        m = re.search('_ch(\d+)x(\d+)_', name)
        res['tube_chirality1'] = m.group(1)
        res['tube_chirality2'] = m.group(2)

    state = None
    skip_str = 4
    last_number = 0
    
    atoms = []
    charges = []

    res['job_name'] = ''

    for l in lines:
        if 'Standard orientation:' in l:
            state = 'parse_orientation_header'
            skip_str = 4
            last_number = 0
            atoms = []
            continue
        if ('Mulliken charges' in l or 'Mulliken atomic charges' in l) and not 'Sum of' in l:
            state = 'parse_atomic_charges'
            charges = []
            skip_str = 1
            continue
        if 'Dipole moment (field-independent basis, Debye)' in l:
            state = 'parse_dipole_moment'
            continue
        if 'Quadrupole moment (field-independent basis, Debye-Ang)' in l:
            state = 'parse_quadrupole_moment'
            continue
        if 'Exact polarizability' in l:
            try:
                res['polarizability_axx'], res['polarizability_axy'], res['polarizability_ayy'], \
                res['polarizability_axz'], res['polarizability_ayz'], res['polarizability_azz'] = map(float, re.findall('[0-9\-]+\.\d{3}', l))
            except:
                print "can't parse polarizability values from {0}".format(l)
            continue
        if l.startswith( ' #' ) and len( res['job_name'] ) == 0:
            res['job_name'] = l[1:]
            continue
        if 'Harmonic frequencies (cm**-1), IR intensities (KM/Mole), Raman scattering' in l:
            state = 'parse_frequencies'
            res['frequencies'] = []
            res['ir_values'] = []
            res['raman_activ'] = []
            skip_str = 6
        if 'SCF Done' in l:
            match = re.match( '[\w\s\(\):-]+=\s*([0-9\.\-]+)', l )
            if match:
                res['energy'] = float(match.group(1))
            continue
        if 'The electronic state is' in l:
            match = re.match( '\s*The electronic state is\s*([\w\-\d]+)\.', l )
            if match:
                res['electronic_state'] = match.group(1)
                res['eigenvalues_occ'] = []
                res['eigenvalues_virt'] = [] 
            continue
        if 'Alpha  occ. eigenvalues' in l:
            res['eigenvalues_occ'].extend(map( float, re.split( '[^0-9\.\-]+', l )[3:] ))
            res['ehomo'] = res['eigenvalues_occ'][-1]
            continue
        if 'Alpha virt. eigenvalues' in l:
            res['eigenvalues_virt'].extend(map( float, re.split( '[^0-9\.\-]+', l )[3:] ))
            res['elumo'] = res['eigenvalues_virt'][0]
            continue
        if 'Temperature ' in l:
            ss = re.split( '[^0-9\.\-]+', l )
            res['temperature'] = ss[1]
            res['pressure'] = ss[3]
        if 'Principal axes and moments of inertia in atomic units' in l:
            state = 'parse_principal_axes'
            skip_str = 2
        if 'Sum of electronic and zero-point Energies' in l:
            res['sum_zero_energies'] = float(re.search('-?\d+\.\d+', l).group(0))
        if 'Sum of electronic and thermal Energies' in l:
            res['sum_thermal_energies'] = float(re.search('-?\d+\.\d+', l).group(0))
        if 'Sum of electronic and thermal Enthalpies' in l:
            res['sum_thermal_enthalpies'] = float(re.search('-?\d+\.\d+', l).group(0))
        if 'Sum of electronic and thermal Free Energies' in l:
            res['sum_thermal_free_energies'] = float(re.search('-?\d+\.\d+', l).group(0))
        if 'E (Thermal)' in l:
            state = 'parse_thermal_e_cv_s'
            skip_str = 2
            

        if state is 'parse_orientation_header':
            skip_str -= 1
            if skip_str == 0:
                state = 'parse_orientation'
            continue
        elif state is 'parse_orientation':
            try:
                number, atom_index, atom_type, x, y, z = scan( l, int, int, int, float, float, float )
            except:
                number = 0

            if last_number > 0 and number <= last_number:
                state = ''
                continue

            last_number = number
            atoms.append( [ atom_index, x, y, z ] )
        elif state is 'parse_atomic_charges':
            if skip_str > 0:
                skip_str -= 1
            else:
                match = re.match( '\s*(\d+)\s*\S+\s*([0-9\.\-]+)', l )
                if match:
                    charges.append( float( match.group( 2 ) ) )
                else:
                    state = ''
        elif state is 'parse_dipole_moment':
            match = re.match( '\s*X=\s*([0-9\.\-]+)\s*Y=\s*([0-9\.\-]+)\s*Z=\s*([0-9\.\-]+)', l )
            if match:
                res['dipole_x'] = float( match.group( 1 ) )
                res['dipole_y'] = float( match.group( 2 ) )
                res['dipole_z'] = float( match.group( 3 ) )
            state = ''
        elif state is 'parse_quadrupole_moment':
            match = re.match( '\s*XX=\s*([0-9\.\-]+)\s*YY=\s*([0-9\.\-]+)\s*ZZ=\s*([0-9\.\-]+)', l )
            if match:
                res['quadrupole_xx'] = float( match.group( 1 ) )
                res['quadrupole_yy'] = float( match.group( 2 ) )
                res['quadrupole_zz'] = float( match.group( 3 ) )
            else:
                match = re.match( '\s*XY=\s*([0-9\.\-]+)\s*XZ=\s*([0-9\.\-]+)\s*YZ=\s*([0-9\.\-]+)', l )
                if match:
                    res['quadrupole_xy'] = float( match.group( 1 ) )
                    res['quadrupole_xz'] = float( match.group( 2 ) )
                    res['quadrupole_yz'] = float( match.group( 3 ) )
                state = ''
        elif state is 'parse_frequencies':
            if skip_str > 0:
                skip_str -= 1
            else:
                v = re.split( '[^0-9\.\-]+', l )
                try:
                    if v[1] == '--':
                        res['frequencies'].extend( map( float, v[2:] ) )
                        state = 'parse_ir'
                        skip_str = 2
                    else:
                        state = ''
                except:
                    state = ''
        elif state is 'parse_ir':
            if skip_str > 0:
                skip_str -= 1
            else:
                v = re.split( '[^0-9\.\-]+', l )
                try:
                    res['ir_values'].extend( map( float, v[2:] ) )
                    state = 'parse_raman'
                except:
                    state = ''
        elif state is 'parse_raman':
            if skip_str > 0:
                skip_str -= 1
            else:
                v = re.split( '[^0-9\.\-]+', l )
                try:
                    res['raman_activ'].extend( map( float, v[2:] ) )
                    state = 'parse_frequencies'
                    skip_str = len( atoms ) + 5
                except:
                    state = ''
        elif state is 'parse_principal_axes':
            if skip_str > 0:
                skip_str -= 1
            else:
                match = re.match('\s*(\w+)\s*-{0,2}\s*([\d\.\-]+)\s*([\d\.\-]+)\s*([\d\.\-]+)', l)
                if match:
                    vec = [float(match.group(i)) for i in range(2,5)] 
                    if match.group(1) == 'EIGENVALUES':
                        res['principal_axes_eigenvalues'] = vec
                    elif match.group(1) == 'X':
                        res['principal_axes_x'] = vec
                    elif match.group(1) == 'Y':
                        res['principal_axes_y'] = vec
                    elif match.group(1) == 'Z':
                        res['principal_axes_z'] = vec
                        state = ''
        elif state is 'parse_thermal_e_cv_s':
            if skip_str > 0:
                skip_str -= 1
            else:
                match = re.match('\s*\w+\s*([\d\.\-]+)\s*([\d\.\-]+)\s*([\d\.\-]+)', l)
                if match:
                    res['thermal_total_e'] = match.group(1)
                    res['thermal_total_cv'] = match.group(2)
                    res['thermal_total_s'] = match.group(3)
                    state = ''
                

    res['atoms'] = []
    if len(charges) > 0:
        res['qC1'] = charges[0]
    for a, c in zip( atoms, charges ):
        res['atoms'].append( ' '.join( map( str, ( a[ 0 ], c, a[ 1 ], a[ 2 ], a[ 3 ] ) ) ) )
        
    return res
    
if __name__ == '__main__':
    with open('data.csv', 'wb') as csvfile:
        writer = csv.writer(csvfile, dialect=csv.excel)
        writeHeader = True
        headerSorted = []
        
        matches = []
        for root, dirnames, filenames in os.walk('out'):
            for filename in fnmatch.filter(filenames, '*.out'):
                matches.append(os.path.join(root, filename))

        for file in matches:
            print 'parsing {0}...'.format(file)
            with open(file, 'rt') as f:
                data = parseGaussianOutput(f, file)
                if writeHeader:
                    headerSorted = data.keys()
                    headerSorted.sort()
                    writer.writerow(headerSorted)
                    writeHeader = False
                writer.writerow([data[x] for x in headerSorted])
