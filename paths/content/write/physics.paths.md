@paths 1
@subject physics | Physics
@chapter document_oscillations | Oscillations from documents

@lesson document_oscillation_reading | Reading a rotating phasor
@template lesson.v1
A rotating arrow represents a sinusoidal displacement. Its vertical
projection is the sine component. This is a geometric signal model.

$$y=A\sin(\theta)$$

Here A is the amplitude, and theta is an angle measured in radians. For a
physical oscillation, theta may represent omega times time plus a phase.
@figure harmonics 0
@parameter amplitude1 1
@parameter frequency1 1
@parameter phase_time 0
@caption The existing 3D phasor begins along the positive horizontal axis; its sine projection is zero. Drag to inspect the diagram.
@practice document_sine_question
@end

@question document_sine_question | Document physics practice
@template choices.v1
@version 1
@goal Find the vertical displacement.
@given y=A\sin(\theta),\quad A=1\,\mathrm{m},\quad\theta=0
@domain Angles are in radians; displacement is measured in metres.
@read document_oscillation_reading
@step 10 | Evaluate the sine at the given angle.
@choice 11 | 0
@choice 12 | 1
@choice 13 | -1
@answer 11
@after \sin(0)=0
@wrong At zero angle the arrow lies horizontally. Its vertical projection is zero.
@why The sine is the vertical component of a unit arrow, which vanishes at angle zero.
@step 20 | Multiply by the amplitude. What is y?
@choice 21 | 1\,\mathrm{m}
@choice 22 | 0\,\mathrm{m}
@choice 23 | -1\,\mathrm{m}
@answer 22
@after y=1\cdot0=0\,\mathrm{m}
@wrong Multiply the amplitude by the sine value, keeping the displacement units.
@why The displacement is zero metres because the sine component is zero.
@end
