@paths 1
@subject trigonometry | Trigonometry
@chapter topic_0032 | Unit Circle Framework

@lesson prod02_trig_full_equations_r | Complete trigonometric solution sets
@template lesson.v2
@block introduction | start | 2.1 | From an equation to every angle
@prose A trigonometric equation asks which angles make the original equality true. A familiar angle is not necessarily the whole answer. We will isolate a coordinate, decide whether that coordinate is possible, construct all argument branches, then translate them back to the requested angles and check completeness.
@endblock
@block definition | terms | 2.2 | Coordinates, arguments and sets
@prose Theta is a real angle in radians. On a circle of radius 1, radian measure equals signed arc length. Positive angles run counterclockwise from the positive horizontal axis; pi radians is a half-turn. At angle u on the unit circle, cosine is the horizontal coordinate x and sine is the vertical coordinate y. Both coordinates lie between -1 and 1.
@display coordinates
(x,y)=(\cos u,\sin u),\qquad x^2+y^2=1
@prose In A f(k theta+phi)+B=C, f means either sine or cosine, A is its nonzero multiplier, B an added constant and C the right side. The frequency k is positive here, and phi is a fixed radian shift. The whole input u=k theta+phi is the argument. It is u, not necessarily theta, whose point lies at the required unit-circle coordinate. Set h=(C-B)/A.
@prose A known solution angle alpha need not be acute. A branch is one repeated family of angles with the same coordinate. The integer n may be negative, zero or positive; Z is the set of all integers and R the set of all real numbers. A period T repeats a value. Braces denote a set, with no order or repetition; the empty-set symbol means there are no solutions. The symbol N counts matching points in one argument turn. M counts retained theta solutions in an interval, or distinct branches in one specified theta period.
@prose S denotes the complete solution set. The notation [0,2pi) includes zero and excludes 2pi. Coterminal angles differ by full turns and have the same circle point. In a general real solution those different angles are all relevant; in a single turn a repeated endpoint must not be counted twice.
@endblock
@block proposition | rule | 2.3 | Coordinate reflections and full turns
@prose Sine is unchanged by reflection across the vertical axis, which sends alpha to pi-alpha. Cosine is unchanged by reflection across the horizontal axis, which sends alpha to -alpha or the coterminal 2pi-alpha. Both functions repeat after 2pi in their argument. At an extreme coordinate the two reflected points coincide; record that branch only once.
@display reflections
\sin(\pi-\alpha)=\sin\alpha,\qquad\cos(-\alpha)=\cos\alpha
@help proof
@prose Fixing either coordinate at h in x squared+y squared=1 leaves the other coordinate squared equal to 1-h squared. This has two distinct real roots for magnitude of h below 1, one root at magnitude 1, and no real root above 1. This geometric count proves completeness once membership of that many distinct circle points has been checked. It is not enough merely to remember one inverse-function value.
@prose Bisecting an equilateral triangle of side 2 gives a right triangle with sides 1, sqrt(3), 2. Dividing by 2 yields sine(pi/6)=1/2 and cosine(pi/3)=1/2. The four quadrants have (x,y) signs (+,+), (-,+), (-,-), (+,-); reflection determines the other half-coordinate points. The axis points determine zero and extreme coordinates. Adding 2n*pi gives every periodic copy.
@endblock
@block proposition | condition | 2.4 | Move the whole argument and the whole interval
@prose Subtract B from both complete sides and divide by nonzero A. These operations are reversible; multiplying and adding back recover the original. Before constructing angles check the range of h. A coordinate outside [-1,1] is impossible, even if the argument has a shift or a doubled frequency.
@display isolation
A f(k\theta+\phi)+B=C\quad\Longleftrightarrow\quad f(u)=\frac{C-B}{A},\qquad u=k\theta+\phi
@prose For positive k, multiplication preserves each interval inequality. Transform both endpoints, including whether they are closed or open. After finding every argument u in that interval, subtract phi and divide by k. For a general real solution, apply the same operations to the full branch expression, including its integer term: the branch period in theta becomes 2pi/k. Omitting that division loses solutions.
@display domain_map
0\le\theta<2\pi\quad\Longleftrightarrow\quad\phi\le u<2k\pi+\phi,\qquad\theta=\frac{u-\phi}{k}
@endblock
@block example | worked | 2.5 | A shifted double-frequency equation
@prose Find all theta in the stated interval for this equation.
@display worked_given
3\sin\left(2\theta-\frac{\pi}{2}\right)+1=-2,\qquad0\le\theta<2\pi
@help hint
@prose Name the complete angle argument u. First isolate sine and check its range. Transform the interval before selecting integer copies of the argument branch.
@help answer
@display
\theta\in\left\{0,\pi\right\}
@help solution
@prose Subtract 1 to get 3sin(u)=-2-1=-3; divide by 3 to get sin(u)=-1. This is the bottom circle point, so there is one distinct argument branch, u=3pi/2+2n*pi. The reflected branch coincides with it.
@display worked_branches
u=2\theta-\frac{\pi}{2}=\frac{3\pi}{2}+2n\pi,\qquad n\in\mathbb{Z}
@prose The original interval maps to -pi/2<=u<7pi/2. Substituting the branch gives -pi/2<=3pi/2+2n*pi<7pi/2. Subtract 3pi/2 and divide by 2pi, which is positive, to get -1<=n<1; the integers are -1 and 0.
@display worked_argument
u\in\left\{-\frac{\pi}{2},\frac{3\pi}{2}\right\},\qquad\theta=\frac{u+\pi/2}{2}\in\left\{0,\pi\right\}
@prose At theta=0 the argument is -pi/2; at theta=pi it is 3pi/2. Both have sine -1 and give 3(-1)+1=-2 in the original equation. The next copy u=7pi/2 would give theta=2pi and is excluded. One point per argument turn, two argument turns, and the invertible argument map prove that these two distinct theta values are all the solutions.
@endblock
@block example | errors | 2.6 | Three ways a complete answer can fail
@prose One valid angle may omit another reflected branch. A correct argument list can become wrong if only the phase or only the periodic term is divided by k. An angle may satisfy the equation but violate the requested interval. Separate membership, completeness and domain checks; none substitutes for the others.
@endblock
@block exercise | practice | 2.7 | Twelve complete routes
@prose Begin with four complete unshifted solves. Continue with four frequency and phase variations, then two general real solution problems and two equations whose range must be examined carefully. Each problem carries its own calculation through the final original-equation check.
@endblock
@block summary | summary | 2.8 | A complete solution has two proofs
@prose Membership means each proposed angle satisfies the original equation and domain. Completeness means no other permitted angle can solve it: use the coordinate-line count, every periodic copy in the transformed interval, and the reversible argument map. For general solutions retain every integer; for impossible coordinates give the range contradiction.
@endblock
@practice prod02_trig_full_equations_q01
@practice prod02_trig_full_equations_q02
@practice prod02_trig_full_equations_q03
@practice prod02_trig_full_equations_q04
@practice prod02_trig_full_equations_q05
@practice prod02_trig_full_equations_q06
@practice prod02_trig_full_equations_q07
@practice prod02_trig_full_equations_q08
@practice prod02_trig_full_equations_q09
@practice prod02_trig_full_equations_q10
@practice prod02_trig_full_equations_q11
@practice prod02_trig_full_equations_q12
@end

@question prod02_trig_full_equations_q01 | Two sine branches
@template choices.v1
@version 1
@goal Find every solution in the stated interval and check the original equation and completeness.
@given 2\sin\left(\theta\right)+1=2,\quad0\le\theta<2\pi
@domain Theta is real and measured in radians. Find every solution in [0,2pi). Write u=theta for the argument. N counts matching points in one argument turn; M counts solutions in the requested theta interval.
@read prod02_trig_full_equations_r
@step 10 | Which equation correctly isolates sine of the whole argument u?
@choice 11 | \sin u=\frac{1}{2}
@choice 13 | \sin u=0
@choice 12 | \sin u=1
@answer 11
@feedback 12 | This coordinate is too large by 1/2. It gives (2)(1)+(1)=3, not 2. Subtract the entire constant and divide the entire right side.
@feedback 13 | This coordinate is too small by 1/2. It gives (2)(0)+(1)=1, not 2. Keep the sign of the divisor 2.
@after u=\theta,\quad\sin u=\frac{1}{2}
@wrong Check the current goal against the original equation, coordinate rule and stated domain.
@why Subtract 1 from both sides: 2sin(u)=2-(1)=1. Divide both sides by the nonzero 2: sin(u)=(1)/(2)=0.5. Multiplication by 2 and addition of 1 reverse these operations.
@step 20 | Which range statement and circle-point count are correct for the isolated coordinate?
@choice 22 | \frac{1}{2}\notin[-1,1],\quad N=0
@choice 23 | \frac{1}{2}\in[-1,1],\quad N=1
@choice 21 | \frac{1}{2}\in[-1,1],\quad N=2
@answer 21
@feedback 22 | 0.5 is in the coordinate range, including its endpoints. Declaring the equation impossible loses genuine solutions.
@feedback 23 | Since -1<0.5<1, the coordinate line cuts the circle twice, not once. Both matching points are needed.
@after \frac{1}{2}\in[-1,1],\quad N=2
@wrong Check the current goal against the original equation, coordinate rule and stated domain.
@why The isolated coordinate 0.5 belongs to [-1,1]. Fixing it in x squared+y squared=1 leaves the other coordinate squared equal to 0.75. That is positive, with two distinct opposite roots and two circle points.
@step 30 | Which formula lists every real argument u at this coordinate, with n any integer?
@choice 31 | u=\frac{\pi}{6}+2n\pi,\quad\text{or}\quad u=\frac{5\pi}{6}+2n\pi,\quad n\in\mathbb{Z}
@choice 33 | u=\frac{\pi}{3}+2n\pi,\quad\text{or}\quad u=\pi+2n\pi,\quad n\in\mathbb{Z}
@choice 32 | u=\frac{\pi}{6}+2n\pi,\quad n\in\mathbb{Z}
@answer 31
@feedback 32 | This omits the valid argument 5pi/6. It has sin value 0.5. An empty formula or a single familiar branch is not complete.
@feedback 33 | Adding pi/6 to the true representatives changes their vertical coordinates. For example, pi/3 does not have sin 0.5; adding full turns cannot repair that mismatch.
@after u=\frac{\pi}{6}+2n\pi,\quad\text{or}\quad u=\frac{5\pi}{6}+2n\pi,\quad n\in\mathbb{Z}
@wrong Check the current goal against the original equation, coordinate rule and stated domain.
@why The matching representatives in one argument turn are pi/6 and 5pi/6. Their sin values are 0.5, using the axis or special-triangle values and vertical-axis reflection. There are exactly 2 distinct circle points at this coordinate. The reflection calculation is pi-pi/6=5pi/6. Adding 2n*pi to each gives every real argument, with n any integer and coincident reflected points counted once.
@step 40 | Which complete theta set remains after applying the half-open interval?
@choice 42 | \theta\in\left\{\frac{5\pi}{6}\right\}
@choice 41 | \theta\in\left\{\frac{\pi}{6},\frac{5\pi}{6}\right\}
@choice 43 | \theta\in\left\{\frac{\pi}{6},\frac{5\pi}{6},2\pi\right\}
@answer 41
@feedback 42 | This omits pi/6, which has sin 0.5 and satisfies 0<=u<2pi. The lower endpoint is included whenever it solves the equation.
@feedback 43 | This adds the excluded upper argument 2pi. It corresponds to theta=2pi, so it is forbidden even if it satisfies the trigonometric equation.
@after \theta\in\left\{\frac{\pi}{6},\frac{5\pi}{6}\right\}
@wrong Check the current goal against the original equation, coordinate rule and stated domain.
@why Filter the periodic formulas by 0<=u<2pi: pi/6+2n*pi uses n=0; 5pi/6+2n*pi uses n=0. This gives pi/6, 5pi/6. No neighboring integer copy fits, because it crosses one of the endpoints. There is 1 complete argument turn, each with 2 matching points, so 2 distinct arguments exhaust the interval. Here u=theta, so these are already the requested angles.
@step 50 | Which original substitution and solution count jointly verify the complete answer?
@choice 53 | (2)(\frac{1}{2})+(1)=2,\quad M=3
@choice 52 | (2)(\frac{1}{2})+(1)=3,\quad M=2
@choice 51 | (2)(\frac{1}{2})+(1)=2,\quad M=2
@answer 51
@feedback 52 | Evaluate the original left side carefully: (2)(0.5)+(1)=2, not 3. The isolated coordinate must reproduce the actual original right side.
@feedback 53 | The displayed substitution is valid, but the count is not. The transformed interval spans 1 full turn, giving 1 times 2=2 distinct angles. Counting one more repeats a point or admits a non-solution.
@after (2)(\frac{1}{2})+(1)=2,\quad M=2
@wrong Check the current goal against the original equation, coordinate rule and stated domain.
@why Each retained theta gives one of the certified argument values pi/6, 5pi/6, all with sin 0.5; hence (2)(0.5)+(1)=2. All 2 angles are in [0,2pi), with no duplicates. The coordinate-line count gives 2 points in the 1 argument turn, and the argument map is invertible. Therefore membership and completeness both hold.
@end

@question prod02_trig_full_equations_q02 | Two cosine branches
@template choices.v1
@version 1
@goal Find every solution in the stated interval and check the original equation and completeness.
@given 2\cos\left(\theta\right)-1=0,\quad0\le\theta<2\pi
@domain Theta is real and measured in radians. Find every solution in [0,2pi). Write u=theta for the argument. N counts matching points in one argument turn; M counts solutions in the requested theta interval.
@read prod02_trig_full_equations_r
@step 10 | Which equation correctly isolates cosine of the whole argument u?
@choice 12 | \cos u=1
@choice 11 | \cos u=\frac{1}{2}
@choice 13 | \cos u=0
@answer 11
@feedback 12 | This coordinate is too large by 1/2. It gives (2)(1)+(-1)=1, not 0. Subtract the entire constant and divide the entire right side.
@feedback 13 | This coordinate is too small by 1/2. It gives (2)(0)+(-1)=-1, not 0. Keep the sign of the divisor 2.
@after u=\theta,\quad\cos u=\frac{1}{2}
@wrong Check the current goal against the original equation, coordinate rule and stated domain.
@why Subtract -1 from both sides: 2cos(u)=0-(-1)=1. Divide both sides by the nonzero 2: cos(u)=(1)/(2)=0.5. Multiplication by 2 and addition of -1 reverse these operations.
@step 20 | Which range statement and circle-point count are correct for the isolated coordinate?
@choice 21 | \frac{1}{2}\in[-1,1],\quad N=2
@choice 23 | \frac{1}{2}\in[-1,1],\quad N=1
@choice 22 | \frac{1}{2}\notin[-1,1],\quad N=0
@answer 21
@feedback 22 | 0.5 is in the coordinate range, including its endpoints. Declaring the equation impossible loses genuine solutions.
@feedback 23 | Since -1<0.5<1, the coordinate line cuts the circle twice, not once. Both matching points are needed.
@after \frac{1}{2}\in[-1,1],\quad N=2
@wrong Check the current goal against the original equation, coordinate rule and stated domain.
@why The isolated coordinate 0.5 belongs to [-1,1]. Fixing it in x squared+y squared=1 leaves the other coordinate squared equal to 0.75. That is positive, with two distinct opposite roots and two circle points.
@step 30 | Which formula lists every real argument u at this coordinate, with n any integer?
@choice 32 | u=\frac{\pi}{3}+2n\pi,\quad n\in\mathbb{Z}
@choice 31 | u=\frac{\pi}{3}+2n\pi,\quad\text{or}\quad u=\frac{5\pi}{3}+2n\pi,\quad n\in\mathbb{Z}
@choice 33 | u=\frac{\pi}{2}+2n\pi,\quad\text{or}\quad u=\frac{11\pi}{6}+2n\pi,\quad n\in\mathbb{Z}
@answer 31
@feedback 32 | This omits the valid argument 5pi/3. It has cos value 0.5. An empty formula or a single familiar branch is not complete.
@feedback 33 | Adding pi/6 to the true representatives changes their horizontal coordinates. For example, pi/2 does not have cos 0.5; adding full turns cannot repair that mismatch.
@after u=\frac{\pi}{3}+2n\pi,\quad\text{or}\quad u=\frac{5\pi}{3}+2n\pi,\quad n\in\mathbb{Z}
@wrong Check the current goal against the original equation, coordinate rule and stated domain.
@why The matching representatives in one argument turn are pi/3 and 5pi/3. Their cos values are 0.5, using the axis or special-triangle values and horizontal-axis reflection. There are exactly 2 distinct circle points at this coordinate. Cosine reflection gives -pi/3; adding 2pi gives 5pi/3 in the representative turn. Adding 2n*pi to each gives every real argument, with n any integer and coincident reflected points counted once.
@step 40 | Which complete theta set remains after applying the half-open interval?
@choice 43 | \theta\in\left\{\frac{\pi}{3},\frac{5\pi}{3},2\pi\right\}
@choice 42 | \theta\in\left\{\frac{5\pi}{3}\right\}
@choice 41 | \theta\in\left\{\frac{\pi}{3},\frac{5\pi}{3}\right\}
@answer 41
@feedback 42 | This omits pi/3, which has cos 0.5 and satisfies 0<=u<2pi. The lower endpoint is included whenever it solves the equation.
@feedback 43 | This adds the excluded upper argument 2pi. It corresponds to theta=2pi, so it is forbidden even if it satisfies the trigonometric equation.
@after \theta\in\left\{\frac{\pi}{3},\frac{5\pi}{3}\right\}
@wrong Check the current goal against the original equation, coordinate rule and stated domain.
@why Filter the periodic formulas by 0<=u<2pi: pi/3+2n*pi uses n=0; 5pi/3+2n*pi uses n=0. This gives pi/3, 5pi/3. No neighboring integer copy fits, because it crosses one of the endpoints. There is 1 complete argument turn, each with 2 matching points, so 2 distinct arguments exhaust the interval. Here u=theta, so these are already the requested angles.
@step 50 | Which original substitution and solution count jointly verify the complete answer?
@choice 51 | (2)(\frac{1}{2})+(-1)=0,\quad M=2
@choice 52 | (2)(\frac{1}{2})+(-1)=1,\quad M=2
@choice 53 | (2)(\frac{1}{2})+(-1)=0,\quad M=3
@answer 51
@feedback 52 | Evaluate the original left side carefully: (2)(0.5)+(-1)=0, not 1. The isolated coordinate must reproduce the actual original right side.
@feedback 53 | The displayed substitution is valid, but the count is not. The transformed interval spans 1 full turn, giving 1 times 2=2 distinct angles. Counting one more repeats a point or admits a non-solution.
@after (2)(\frac{1}{2})+(-1)=0,\quad M=2
@wrong Check the current goal against the original equation, coordinate rule and stated domain.
@why Each retained theta gives one of the certified argument values pi/3, 5pi/3, all with cos 0.5; hence (2)(0.5)+(-1)=0. All 2 angles are in [0,2pi), with no duplicates. The coordinate-line count gives 2 points in the 1 argument turn, and the argument map is invertible. Therefore membership and completeness both hold.
@end

@question prod02_trig_full_equations_q03 | A signed sine equation
@template choices.v1
@version 1
@goal Find every solution in the stated interval and check the original equation and completeness.
@given -2\sin\left(\theta\right)+1=-1,\quad0\le\theta<2\pi
@domain Theta is real and measured in radians. Find every solution in [0,2pi). Write u=theta for the argument. N counts matching points in one argument turn; M counts solutions in the requested theta interval.
@read prod02_trig_full_equations_r
@step 10 | Which equation correctly isolates sine of the whole argument u?
@choice 13 | \sin u=\frac{1}{2}
@choice 12 | \sin u=\frac{3}{2}
@choice 11 | \sin u=1
@answer 11
@feedback 12 | This coordinate is too large by 1/2. It gives (-2)(1.5)+(1)=-2, not -1. Subtract the entire constant and divide the entire right side.
@feedback 13 | This coordinate is too small by 1/2. It gives (-2)(0.5)+(1)=0, not -1. Keep the sign of the divisor -2.
@after u=\theta,\quad\sin u=1
@wrong Check the current goal against the original equation, coordinate rule and stated domain.
@why Subtract 1 from both sides: -2sin(u)=-1-(1)=-2. Divide both sides by the nonzero -2: sin(u)=(-2)/(-2)=1. Multiplication by -2 and addition of 1 reverse these operations.
@step 20 | Which range statement and circle-point count are correct for the isolated coordinate?
@choice 22 | 1\notin[-1,1],\quad N=0
@choice 21 | 1\in[-1,1],\quad N=1
@choice 23 | 1\in[-1,1],\quad N=2
@answer 21
@feedback 22 | 1 is in the coordinate range, including its endpoints. Declaring the equation impossible loses genuine solutions.
@feedback 23 | At coordinate 1, the line touches the circle at one extreme point; reflection returns that same point, not a second solution.
@after 1\in[-1,1],\quad N=1
@wrong Check the current goal against the original equation, coordinate rule and stated domain.
@why The isolated coordinate 1 belongs to [-1,1]. Fixing it in x squared+y squared=1 leaves the other coordinate squared equal to 0. That is zero, with one real root and one circle point.
@step 30 | Which formula lists every real argument u at this coordinate, with n any integer?
@choice 33 | u=\frac{2\pi}{3}+2n\pi,\quad n\in\mathbb{Z}
@choice 32 | u\in\varnothing
@choice 31 | u=\frac{\pi}{2}+2n\pi,\quad n\in\mathbb{Z}
@answer 31
@feedback 32 | This omits the valid argument pi/2. It has sin value 1. This empty formula loses the only argument branch.
@feedback 33 | Adding pi/6 to the true representative changes its vertical coordinate. For example, 2pi/3 does not have sin 1; adding full turns cannot repair that mismatch.
@after u=\frac{\pi}{2}+2n\pi,\quad n\in\mathbb{Z}
@wrong Check the current goal against the original equation, coordinate rule and stated domain.
@why The matching representative in one argument turn is pi/2. Its sin value is 1, using the axis or special-triangle values and vertical-axis reflection. There is exactly 1 distinct circle point at this coordinate. The reflection is pi-pi/2=pi/2, the same extreme point, not a second branch. Adding 2n*pi to it gives every real argument, with n any integer and coincident reflected points counted once.
@step 40 | Which complete theta set remains after applying the half-open interval?
@choice 41 | \theta\in\left\{\frac{\pi}{2}\right\}
@choice 42 | \theta\in\varnothing
@choice 43 | \theta\in\left\{\frac{\pi}{2},2\pi\right\}
@answer 41
@feedback 42 | This omits pi/2, which has sin 1 and satisfies 0<=u<2pi. The lower endpoint is included whenever it solves the equation.
@feedback 43 | This adds the excluded upper argument 2pi. It corresponds to theta=2pi, so it is forbidden even if it satisfies the trigonometric equation.
@after \theta\in\left\{\frac{\pi}{2}\right\}
@wrong Check the current goal against the original equation, coordinate rule and stated domain.
@why Filter the periodic formulas by 0<=u<2pi: pi/2+2n*pi uses n=0. This gives pi/2. No neighboring integer copy fits, because it crosses one of the endpoints. There is 1 complete argument turn, each with 1 matching point, so 1 distinct argument exhausts the interval. Here u=theta, so these are already the requested angles.
@step 50 | Which original substitution and solution count jointly verify the complete answer?
@choice 53 | (-2)(1)+(1)=-1,\quad M=2
@choice 51 | (-2)(1)+(1)=-1,\quad M=1
@choice 52 | (-2)(1)+(1)=0,\quad M=1
@answer 51
@feedback 52 | Evaluate the original left side carefully: (-2)(1)+(1)=-1, not 0. The isolated coordinate must reproduce the actual original right side.
@feedback 53 | The displayed substitution is valid, but the count is not. The transformed interval spans 1 full turn, giving 1 times 1=1 distinct angle. Counting one more repeats a point or admits a non-solution.
@after (-2)(1)+(1)=-1,\quad M=1
@wrong Check the current goal against the original equation, coordinate rule and stated domain.
@why The retained theta gives the certified argument value pi/2, with sin 1; hence (-2)(1)+(1)=-1. The 1 angle is in [0,2pi), with no duplicates. The coordinate-line count gives 1 point in the 1 argument turn, and the argument map is invertible. Therefore membership and completeness both hold.
@end

@question prod02_trig_full_equations_q04 | A scaled cosine equation
@template choices.v1
@version 1
@goal Find every solution in the stated interval and check the original equation and completeness.
@given 3\cos\left(\theta\right)-2=1,\quad0\le\theta<2\pi
@domain Theta is real and measured in radians. Find every solution in [0,2pi). Write u=theta for the argument. N counts matching points in one argument turn; M counts solutions in the requested theta interval.
@read prod02_trig_full_equations_r
@step 10 | Which equation correctly isolates cosine of the whole argument u?
@choice 12 | \cos u=\frac{3}{2}
@choice 11 | \cos u=1
@choice 13 | \cos u=\frac{1}{2}
@answer 11
@feedback 12 | This coordinate is too large by 1/2. It gives (3)(1.5)+(-2)=2.5, not 1. Subtract the entire constant and divide the entire right side.
@feedback 13 | This coordinate is too small by 1/2. It gives (3)(0.5)+(-2)=-0.5, not 1. Keep the sign of the divisor 3.
@after u=\theta,\quad\cos u=1
@wrong Check the current goal against the original equation, coordinate rule and stated domain.
@why Subtract -2 from both sides: 3cos(u)=1-(-2)=3. Divide both sides by the nonzero 3: cos(u)=(3)/(3)=1. Multiplication by 3 and addition of -2 reverse these operations.
@step 20 | Which range statement and circle-point count are correct for the isolated coordinate?
@choice 23 | 1\in[-1,1],\quad N=2
@choice 22 | 1\notin[-1,1],\quad N=0
@choice 21 | 1\in[-1,1],\quad N=1
@answer 21
@feedback 22 | 1 is in the coordinate range, including its endpoints. Declaring the equation impossible loses genuine solutions.
@feedback 23 | At coordinate 1, the line touches the circle at one extreme point; reflection returns that same point, not a second solution.
@after 1\in[-1,1],\quad N=1
@wrong Check the current goal against the original equation, coordinate rule and stated domain.
@why The isolated coordinate 1 belongs to [-1,1]. Fixing it in x squared+y squared=1 leaves the other coordinate squared equal to 0. That is zero, with one real root and one circle point.
@step 30 | Which formula lists every real argument u at this coordinate, with n any integer?
@choice 31 | u=0+2n\pi,\quad n\in\mathbb{Z}
@choice 32 | u\in\varnothing
@choice 33 | u=\frac{\pi}{6}+2n\pi,\quad n\in\mathbb{Z}
@answer 31
@feedback 32 | This omits the valid argument 0. It has cos value 1. This empty formula loses the only argument branch.
@feedback 33 | Adding pi/6 to the true representative changes its horizontal coordinate. For example, pi/6 does not have cos 1; adding full turns cannot repair that mismatch.
@after u=0+2n\pi,\quad n\in\mathbb{Z}
@wrong Check the current goal against the original equation, coordinate rule and stated domain.
@why The matching representative in one argument turn is 0. Its cos value is 1, using the axis or special-triangle values and horizontal-axis reflection. There is exactly 1 distinct circle point at this coordinate. Cosine reflection sends zero to -0=0; the repeated 2pi direction is not another branch. Adding 2n*pi to it gives every real argument, with n any integer and coincident reflected points counted once.
@step 40 | Which complete theta set remains after applying the half-open interval?
@choice 43 | \theta\in\left\{0,2\pi\right\}
@choice 41 | \theta\in\left\{0\right\}
@choice 42 | \theta\in\varnothing
@answer 41
@feedback 42 | This omits 0, which has cos 1 and satisfies 0<=u<2pi. The lower endpoint is included whenever it solves the equation.
@feedback 43 | This adds the excluded upper argument 2pi. It corresponds to theta=2pi, so it is forbidden even if it satisfies the trigonometric equation.
@after \theta\in\left\{0\right\}
@wrong Check the current goal against the original equation, coordinate rule and stated domain.
@why Filter the periodic formulas by 0<=u<2pi: 0+2n*pi uses n=0. This gives 0. No neighboring integer copy fits, because it crosses one of the endpoints. There is 1 complete argument turn, each with 1 matching point, so 1 distinct argument exhausts the interval. Here u=theta, so these are already the requested angles.
@step 50 | Which original substitution and solution count jointly verify the complete answer?
@choice 52 | (3)(1)+(-2)=2,\quad M=1
@choice 53 | (3)(1)+(-2)=1,\quad M=2
@choice 51 | (3)(1)+(-2)=1,\quad M=1
@answer 51
@feedback 52 | Evaluate the original left side carefully: (3)(1)+(-2)=1, not 2. The isolated coordinate must reproduce the actual original right side.
@feedback 53 | The displayed substitution is valid, but the count is not. The transformed interval spans 1 full turn, giving 1 times 1=1 distinct angle. Counting one more repeats a point or admits a non-solution.
@after (3)(1)+(-2)=1,\quad M=1
@wrong Check the current goal against the original equation, coordinate rule and stated domain.
@why The retained theta gives the certified argument value 0, with cos 1; hence (3)(1)+(-2)=1. The 1 angle is in [0,2pi), with no duplicates. The coordinate-line count gives 1 point in the 1 argument turn, and the argument map is invertible. Therefore membership and completeness both hold.
@end

@question prod02_trig_full_equations_q05 | A double-angle sine equation
@template choices.v1
@version 1
@goal Find every solution in the stated interval and check the original equation and completeness.
@given 2\sin\left(2\theta\right)-1=-1,\quad0\le\theta<2\pi
@domain Theta is real and measured in radians. Find every solution in [0,2pi). Write u=2theta for the argument. N counts matching points in one argument turn; M counts solutions in the requested theta interval.
@read prod02_trig_full_equations_r
@step 10 | Which equation correctly isolates sine of the whole argument u?
@choice 13 | \sin u=-\frac{1}{2}
@choice 12 | \sin u=\frac{1}{2}
@choice 11 | \sin u=0
@answer 11
@feedback 12 | This coordinate is too large by 1/2. It gives (2)(0.5)+(-1)=0, not -1. Subtract the entire constant and divide the entire right side.
@feedback 13 | This coordinate is too small by 1/2. It gives (2)(-0.5)+(-1)=-2, not -1. Keep the sign of the divisor 2.
@after u=2\theta,\quad\sin u=0
@wrong Check the current goal against the original equation, coordinate rule and stated domain.
@why Subtract -1 from both sides: 2sin(u)=-1-(-1)=0. Divide both sides by the nonzero 2: sin(u)=(0)/(2)=0. Multiplication by 2 and addition of -1 reverse these operations.
@step 20 | Which range statement and circle-point count are correct for the isolated coordinate?
@choice 21 | 0\in[-1,1],\quad N=2
@choice 22 | 0\notin[-1,1],\quad N=0
@choice 23 | 0\in[-1,1],\quad N=1
@answer 21
@feedback 22 | 0 is in the coordinate range, including its endpoints. Declaring the equation impossible loses genuine solutions.
@feedback 23 | Since -1<0<1, the coordinate line cuts the circle twice, not once. Both matching points are needed.
@after 0\in[-1,1],\quad N=2
@wrong Check the current goal against the original equation, coordinate rule and stated domain.
@why The isolated coordinate 0 belongs to [-1,1]. Fixing it in x squared+y squared=1 leaves the other coordinate squared equal to 1. That is positive, with two distinct opposite roots and two circle points.
@step 30 | Which formula lists every real argument u at this coordinate, with n any integer?
@choice 33 | u=\frac{\pi}{6}+2n\pi,\quad\text{or}\quad u=\frac{7\pi}{6}+2n\pi,\quad n\in\mathbb{Z}
@choice 31 | u=0+2n\pi,\quad\text{or}\quad u=\pi+2n\pi,\quad n\in\mathbb{Z}
@choice 32 | u=0+2n\pi,\quad n\in\mathbb{Z}
@answer 31
@feedback 32 | This omits the valid argument pi. It has sin value 0. An empty formula or a single familiar branch is not complete.
@feedback 33 | Adding pi/6 to the true representatives changes their vertical coordinates. For example, pi/6 does not have sin 0; adding full turns cannot repair that mismatch.
@after u=0+2n\pi,\quad\text{or}\quad u=\pi+2n\pi,\quad n\in\mathbb{Z}
@wrong Check the current goal against the original equation, coordinate rule and stated domain.
@why The matching representatives in one argument turn are 0 and pi. Their sin values are 0, using the axis or special-triangle values and vertical-axis reflection. There are exactly 2 distinct circle points at this coordinate. Sine reflection sends zero to pi-0=pi, producing the other axis point. Adding 2n*pi to each gives every real argument, with n any integer and coincident reflected points counted once.
@step 40 | What interval for u is exactly equivalent to the original theta interval?
@choice 42 | \frac{\pi}{2}\le u<4\pi
@choice 43 | 0\le u<5\pi
@choice 41 | 0\le u<4\pi
@answer 41
@feedback 42 | The lower endpoint must be 2(0)+(0)=0. This option shifts that endpoint by an extra pi/2 and discards part of the original theta domain.
@feedback 43 | The upper endpoint is 2(2pi)+(0)=4pi, still excluded. This option extends it by pi, admitting arguments from outside the requested theta interval.
@after 0\le u<4\pi
@wrong Check the current goal against the original equation, coordinate rule and stated domain.
@why Since k=2 is positive, multiplying 0<=theta<2pi by 2 preserves both inequality directions. Add 0 to both bounds: the lower endpoint is 0 and the upper is 4pi. The lower inequality stays closed; the upper stays strict.
@step 50 | Which complete argument set remains after applying the half-open interval?
@choice 51 | u\in\left\{0,\pi,2\pi,3\pi\right\}
@choice 53 | u\in\left\{0,\pi,2\pi,3\pi,4\pi\right\}
@choice 52 | u\in\left\{\pi,2\pi,3\pi\right\}
@answer 51
@feedback 52 | This omits 0, which has sin 0 and satisfies 0<=u<4pi. The lower endpoint is included whenever it solves the equation.
@feedback 53 | This adds the excluded upper argument 4pi. It corresponds to theta=2pi, so it is forbidden even if it satisfies the trigonometric equation.
@after u\in\left\{0,\pi,2\pi,3\pi\right\}
@wrong Check the current goal against the original equation, coordinate rule and stated domain.
@why Filter the periodic formulas by 0<=u<4pi: 0+2n*pi uses n=0,1; pi+2n*pi uses n=0,1. This gives 0, pi, 2pi, 3pi. No neighboring integer copy fits, because it crosses one of the endpoints. There are 2 complete argument turns, each with 2 matching points, so 4 distinct arguments exhaust the interval.
@step 60 | Which theta set results from undoing the complete argument transformation?
@choice 62 | \theta\in\left\{0,\pi,2\pi,3\pi\right\}
@choice 61 | \theta\in\left\{0,\frac{\pi}{2},\pi,\frac{3\pi}{2}\right\}
@choice 63 | \theta\in\left\{\frac{\pi}{12},\frac{7\pi}{12},\frac{13\pi}{12},\frac{19\pi}{12}\right\}
@answer 61
@feedback 62 | These are the u values, not the theta values. Use theta=(u-(0))/2; undo the frequency instead of relabelling the argument.
@feedback 63 | Each listed theta is too large by pi/12. The required inverse is exactly theta=(u-(0))/2, with no additional offset.
@after \theta\in\left\{0,\frac{\pi}{2},\pi,\frac{3\pi}{2}\right\}
@wrong Check the current goal against the original equation, coordinate rule and stated domain.
@why u=0 gives theta=(0-(0))/2=0; u=pi gives theta=(pi-(0))/2=pi/2; u=2pi gives theta=(2pi-(0))/2=pi; u=3pi gives theta=(3pi-(0))/2=3pi/2. This invertible linear map preserves all 4 distinct solutions and sends the argument interval back to [0,2pi).
@step 70 | Which original substitution and solution count jointly verify the complete answer?
@choice 73 | (2)(0)+(-1)=-1,\quad M=5
@choice 72 | (2)(0)+(-1)=0,\quad M=4
@choice 71 | (2)(0)+(-1)=-1,\quad M=4
@answer 71
@feedback 72 | Evaluate the original left side carefully: (2)(0)+(-1)=-1, not 0. The isolated coordinate must reproduce the actual original right side.
@feedback 73 | The displayed substitution is valid, but the count is not. The transformed interval spans 2 full turns, giving 2 times 2=4 distinct angles. Counting one more repeats a point or admits a non-solution.
@after (2)(0)+(-1)=-1,\quad M=4
@wrong Check the current goal against the original equation, coordinate rule and stated domain.
@why Each retained theta gives one of the certified argument values 0, pi, 2pi, 3pi, all with sin 0; hence (2)(0)+(-1)=-1. All 4 angles are in [0,2pi), with no duplicates. The coordinate-line count gives 2 points in each of 2 argument turns, and the argument map is invertible. Therefore membership and completeness both hold.
@end

@question prod02_trig_full_equations_q06 | A double-angle cosine equation
@template choices.v1
@version 1
@goal Find every solution in the stated interval and check the original equation and completeness.
@given -2\cos\left(2\theta\right)+1=3,\quad0\le\theta<2\pi
@domain Theta is real and measured in radians. Find every solution in [0,2pi). Write u=2theta for the argument. N counts matching points in one argument turn; M counts solutions in the requested theta interval.
@read prod02_trig_full_equations_r
@step 10 | Which equation correctly isolates cosine of the whole argument u?
@choice 11 | \cos u=-1
@choice 12 | \cos u=-\frac{1}{2}
@choice 13 | \cos u=-\frac{3}{2}
@answer 11
@feedback 12 | This coordinate is too large by 1/2. It gives (-2)(-0.5)+(1)=2, not 3. Subtract the entire constant and divide the entire right side.
@feedback 13 | This coordinate is too small by 1/2. It gives (-2)(-1.5)+(1)=4, not 3. Keep the sign of the divisor -2.
@after u=2\theta,\quad\cos u=-1
@wrong Check the current goal against the original equation, coordinate rule and stated domain.
@why Subtract 1 from both sides: -2cos(u)=3-(1)=2. Divide both sides by the nonzero -2: cos(u)=(2)/(-2)=-1. Multiplication by -2 and addition of 1 reverse these operations.
@step 20 | Which range statement and circle-point count are correct for the isolated coordinate?
@choice 23 | -1\in[-1,1],\quad N=2
@choice 21 | -1\in[-1,1],\quad N=1
@choice 22 | -1\notin[-1,1],\quad N=0
@answer 21
@feedback 22 | -1 is in the coordinate range, including its endpoints. Declaring the equation impossible loses genuine solutions.
@feedback 23 | At coordinate -1, the line touches the circle at one extreme point; reflection returns that same point, not a second solution.
@after -1\in[-1,1],\quad N=1
@wrong Check the current goal against the original equation, coordinate rule and stated domain.
@why The isolated coordinate -1 belongs to [-1,1]. Fixing it in x squared+y squared=1 leaves the other coordinate squared equal to 0. That is zero, with one real root and one circle point.
@step 30 | Which formula lists every real argument u at this coordinate, with n any integer?
@choice 32 | u\in\varnothing
@choice 33 | u=\frac{7\pi}{6}+2n\pi,\quad n\in\mathbb{Z}
@choice 31 | u=\pi+2n\pi,\quad n\in\mathbb{Z}
@answer 31
@feedback 32 | This omits the valid argument pi. It has cos value -1. This empty formula loses the only argument branch.
@feedback 33 | Adding pi/6 to the true representative changes its horizontal coordinate. For example, 7pi/6 does not have cos -1; adding full turns cannot repair that mismatch.
@after u=\pi+2n\pi,\quad n\in\mathbb{Z}
@wrong Check the current goal against the original equation, coordinate rule and stated domain.
@why The matching representative in one argument turn is pi. Its cos value is -1, using the axis or special-triangle values and horizontal-axis reflection. There is exactly 1 distinct circle point at this coordinate. Cosine reflection gives -pi, and -pi+2pi=pi, so the extreme branch is counted once. Adding 2n*pi to it gives every real argument, with n any integer and coincident reflected points counted once.
@step 40 | What interval for u is exactly equivalent to the original theta interval?
@choice 41 | 0\le u<4\pi
@choice 43 | 0\le u<5\pi
@choice 42 | \frac{\pi}{2}\le u<4\pi
@answer 41
@feedback 42 | The lower endpoint must be 2(0)+(0)=0. This option shifts that endpoint by an extra pi/2 and discards part of the original theta domain.
@feedback 43 | The upper endpoint is 2(2pi)+(0)=4pi, still excluded. This option extends it by pi, admitting arguments from outside the requested theta interval.
@after 0\le u<4\pi
@wrong Check the current goal against the original equation, coordinate rule and stated domain.
@why Since k=2 is positive, multiplying 0<=theta<2pi by 2 preserves both inequality directions. Add 0 to both bounds: the lower endpoint is 0 and the upper is 4pi. The lower inequality stays closed; the upper stays strict.
@step 50 | Which complete argument set remains after applying the half-open interval?
@choice 52 | u\in\left\{3\pi\right\}
@choice 51 | u\in\left\{\pi,3\pi\right\}
@choice 53 | u\in\left\{\pi,3\pi,4\pi\right\}
@answer 51
@feedback 52 | This omits pi, which has cos -1 and satisfies 0<=u<4pi. The lower endpoint is included whenever it solves the equation.
@feedback 53 | This adds the excluded upper argument 4pi. It corresponds to theta=2pi, so it is forbidden even if it satisfies the trigonometric equation.
@after u\in\left\{\pi,3\pi\right\}
@wrong Check the current goal against the original equation, coordinate rule and stated domain.
@why Filter the periodic formulas by 0<=u<4pi: pi+2n*pi uses n=0,1. This gives pi, 3pi. No neighboring integer copy fits, because it crosses one of the endpoints. There are 2 complete argument turns, each with 1 matching point, so 2 distinct arguments exhaust the interval.
@step 60 | Which theta set results from undoing the complete argument transformation?
@choice 63 | \theta\in\left\{\frac{7\pi}{12},\frac{19\pi}{12}\right\}
@choice 62 | \theta\in\left\{\pi,3\pi\right\}
@choice 61 | \theta\in\left\{\frac{\pi}{2},\frac{3\pi}{2}\right\}
@answer 61
@feedback 62 | These are the u values, not the theta values. Use theta=(u-(0))/2; undo the frequency instead of relabelling the argument.
@feedback 63 | Each listed theta is too large by pi/12. The required inverse is exactly theta=(u-(0))/2, with no additional offset.
@after \theta\in\left\{\frac{\pi}{2},\frac{3\pi}{2}\right\}
@wrong Check the current goal against the original equation, coordinate rule and stated domain.
@why u=pi gives theta=(pi-(0))/2=pi/2; u=3pi gives theta=(3pi-(0))/2=3pi/2. This invertible linear map preserves all 2 distinct solutions and sends the argument interval back to [0,2pi).
@step 70 | Which original substitution and solution count jointly verify the complete answer?
@choice 71 | (-2)(-1)+(1)=3,\quad M=2
@choice 72 | (-2)(-1)+(1)=4,\quad M=2
@choice 73 | (-2)(-1)+(1)=3,\quad M=3
@answer 71
@feedback 72 | Evaluate the original left side carefully: (-2)(-1)+(1)=3, not 4. The isolated coordinate must reproduce the actual original right side.
@feedback 73 | The displayed substitution is valid, but the count is not. The transformed interval spans 2 full turns, giving 2 times 1=2 distinct angles. Counting one more repeats a point or admits a non-solution.
@after (-2)(-1)+(1)=3,\quad M=2
@wrong Check the current goal against the original equation, coordinate rule and stated domain.
@why Each retained theta gives one of the certified argument values pi, 3pi, all with cos -1; hence (-2)(-1)+(1)=3. All 2 angles are in [0,2pi), with no duplicates. The coordinate-line count gives 1 point in each of 2 argument turns, and the argument map is invertible. Therefore membership and completeness both hold.
@end

@question prod02_trig_full_equations_q07 | Sine with an argument shift
@template choices.v1
@version 1
@goal Find every solution in the stated interval and check the original equation and completeness.
@given 2\sin\left(\theta+\frac{\pi}{2}\right)+2=1,\quad0\le\theta<2\pi
@domain Theta is real and measured in radians. Find every solution in [0,2pi). Write u=theta+pi/2 for the argument. N counts matching points in one argument turn; M counts solutions in the requested theta interval.
@read prod02_trig_full_equations_r
@step 10 | Which equation correctly isolates sine of the whole argument u?
@choice 13 | \sin u=-1
@choice 12 | \sin u=0
@choice 11 | \sin u=-\frac{1}{2}
@answer 11
@feedback 12 | This coordinate is too large by 1/2. It gives (2)(0)+(2)=2, not 1. Subtract the entire constant and divide the entire right side.
@feedback 13 | This coordinate is too small by 1/2. It gives (2)(-1)+(2)=0, not 1. Keep the sign of the divisor 2.
@after u=\theta+\frac{\pi}{2},\quad\sin u=-\frac{1}{2}
@wrong Check the current goal against the original equation, coordinate rule and stated domain.
@why Subtract 2 from both sides: 2sin(u)=1-(2)=-1. Divide both sides by the nonzero 2: sin(u)=(-1)/(2)=-0.5. Multiplication by 2 and addition of 2 reverse these operations.
@step 20 | Which range statement and circle-point count are correct for the isolated coordinate?
@choice 22 | -\frac{1}{2}\notin[-1,1],\quad N=0
@choice 23 | -\frac{1}{2}\in[-1,1],\quad N=1
@choice 21 | -\frac{1}{2}\in[-1,1],\quad N=2
@answer 21
@feedback 22 | -0.5 is in the coordinate range, including its endpoints. Declaring the equation impossible loses genuine solutions.
@feedback 23 | Since -1<-0.5<1, the coordinate line cuts the circle twice, not once. Both matching points are needed.
@after -\frac{1}{2}\in[-1,1],\quad N=2
@wrong Check the current goal against the original equation, coordinate rule and stated domain.
@why The isolated coordinate -0.5 belongs to [-1,1]. Fixing it in x squared+y squared=1 leaves the other coordinate squared equal to 0.75. That is positive, with two distinct opposite roots and two circle points.
@step 30 | Which formula lists every real argument u at this coordinate, with n any integer?
@choice 31 | u=\frac{7\pi}{6}+2n\pi,\quad\text{or}\quad u=\frac{11\pi}{6}+2n\pi,\quad n\in\mathbb{Z}
@choice 33 | u=\frac{4\pi}{3}+2n\pi,\quad\text{or}\quad u=2\pi+2n\pi,\quad n\in\mathbb{Z}
@choice 32 | u=\frac{7\pi}{6}+2n\pi,\quad n\in\mathbb{Z}
@answer 31
@feedback 32 | This omits the valid argument 11pi/6. It has sin value -0.5. An empty formula or a single familiar branch is not complete.
@feedback 33 | Adding pi/6 to the true representatives changes their vertical coordinates. For example, 4pi/3 does not have sin -0.5; adding full turns cannot repair that mismatch.
@after u=\frac{7\pi}{6}+2n\pi,\quad\text{or}\quad u=\frac{11\pi}{6}+2n\pi,\quad n\in\mathbb{Z}
@wrong Check the current goal against the original equation, coordinate rule and stated domain.
@why The matching representatives in one argument turn are 7pi/6 and 11pi/6. Their sin values are -0.5, using the axis or special-triangle values and vertical-axis reflection. There are exactly 2 distinct circle points at this coordinate. The sine reflection is pi-7pi/6=-pi/6; adding 2pi gives 11pi/6. Adding 2n*pi to each gives every real argument, with n any integer and coincident reflected points counted once.
@step 40 | What interval for u is exactly equivalent to the original theta interval?
@choice 42 | \pi\le u<\frac{5\pi}{2}
@choice 41 | \frac{\pi}{2}\le u<\frac{5\pi}{2}
@choice 43 | \frac{\pi}{2}\le u<\frac{7\pi}{2}
@answer 41
@feedback 42 | The lower endpoint must be 1(0)+(pi/2)=pi/2. This option shifts that endpoint by an extra pi/2 and discards part of the original theta domain.
@feedback 43 | The upper endpoint is 1(2pi)+(pi/2)=5pi/2, still excluded. This option extends it by pi, admitting arguments from outside the requested theta interval.
@after \frac{\pi}{2}\le u<\frac{5\pi}{2}
@wrong Check the current goal against the original equation, coordinate rule and stated domain.
@why Since k=1 is positive, multiplying 0<=theta<2pi by 1 preserves both inequality directions. Add pi/2 to both bounds: the lower endpoint is pi/2 and the upper is 5pi/2. The lower inequality stays closed; the upper stays strict.
@step 50 | Which complete argument set remains after applying the half-open interval?
@choice 53 | u\in\left\{\frac{7\pi}{6},\frac{11\pi}{6},\frac{5\pi}{2}\right\}
@choice 52 | u\in\left\{\frac{11\pi}{6}\right\}
@choice 51 | u\in\left\{\frac{7\pi}{6},\frac{11\pi}{6}\right\}
@answer 51
@feedback 52 | This omits 7pi/6, which has sin -0.5 and satisfies pi/2<=u<5pi/2. The lower endpoint is included whenever it solves the equation.
@feedback 53 | This adds the excluded upper argument 5pi/2. It corresponds to theta=2pi, so it is forbidden even if it satisfies the trigonometric equation.
@after u\in\left\{\frac{7\pi}{6},\frac{11\pi}{6}\right\}
@wrong Check the current goal against the original equation, coordinate rule and stated domain.
@why Filter the periodic formulas by pi/2<=u<5pi/2: 7pi/6+2n*pi uses n=0; 11pi/6+2n*pi uses n=0. This gives 7pi/6, 11pi/6. No neighboring integer copy fits, because it crosses one of the endpoints. There is 1 complete argument turn, each with 2 matching points, so 2 distinct arguments exhaust the interval.
@step 60 | Which theta set results from undoing the complete argument transformation?
@choice 61 | \theta\in\left\{\frac{2\pi}{3},\frac{4\pi}{3}\right\}
@choice 62 | \theta\in\left\{\frac{7\pi}{6},\frac{11\pi}{6}\right\}
@choice 63 | \theta\in\left\{\frac{3\pi}{4},\frac{17\pi}{12}\right\}
@answer 61
@feedback 62 | These are the u values, not the theta values. Use theta=(u-(pi/2))/1; undo the phase shift instead of relabelling the argument.
@feedback 63 | Each listed theta is too large by pi/12. The required inverse is exactly theta=(u-(pi/2))/1, with no additional offset.
@after \theta\in\left\{\frac{2\pi}{3},\frac{4\pi}{3}\right\}
@wrong Check the current goal against the original equation, coordinate rule and stated domain.
@why u=7pi/6 gives theta=(7pi/6-(pi/2))/1=2pi/3; u=11pi/6 gives theta=(11pi/6-(pi/2))/1=4pi/3. This invertible linear map preserves all 2 distinct solutions and sends the argument interval back to [0,2pi).
@step 70 | Which original substitution and solution count jointly verify the complete answer?
@choice 73 | (2)(-\frac{1}{2})+(2)=1,\quad M=3
@choice 71 | (2)(-\frac{1}{2})+(2)=1,\quad M=2
@choice 72 | (2)(-\frac{1}{2})+(2)=2,\quad M=2
@answer 71
@feedback 72 | Evaluate the original left side carefully: (2)(-0.5)+(2)=1, not 2. The isolated coordinate must reproduce the actual original right side.
@feedback 73 | The displayed substitution is valid, but the count is not. The transformed interval spans 1 full turn, giving 1 times 2=2 distinct angles. Counting one more repeats a point or admits a non-solution.
@after (2)(-\frac{1}{2})+(2)=1,\quad M=2
@wrong Check the current goal against the original equation, coordinate rule and stated domain.
@why Each retained theta gives one of the certified argument values 7pi/6, 11pi/6, all with sin -0.5; hence (2)(-0.5)+(2)=1. All 2 angles are in [0,2pi), with no duplicates. The coordinate-line count gives 2 points in the 1 argument turn, and the argument map is invertible. Therefore membership and completeness both hold.
@end

@question prod02_trig_full_equations_q08 | Cosine with an argument shift
@template choices.v1
@version 1
@goal Find every solution in the stated interval and check the original equation and completeness.
@given 3\cos\left(\theta-\frac{\pi}{2}\right)-1=-1,\quad0\le\theta<2\pi
@domain Theta is real and measured in radians. Find every solution in [0,2pi). Write u=theta-pi/2 for the argument. N counts matching points in one argument turn; M counts solutions in the requested theta interval.
@read prod02_trig_full_equations_r
@step 10 | Which equation correctly isolates cosine of the whole argument u?
@choice 11 | \cos u=0
@choice 12 | \cos u=\frac{1}{2}
@choice 13 | \cos u=-\frac{1}{2}
@answer 11
@feedback 12 | This coordinate is too large by 1/2. It gives (3)(0.5)+(-1)=0.5, not -1. Subtract the entire constant and divide the entire right side.
@feedback 13 | This coordinate is too small by 1/2. It gives (3)(-0.5)+(-1)=-2.5, not -1. Keep the sign of the divisor 3.
@after u=\theta-\frac{\pi}{2},\quad\cos u=0
@wrong Check the current goal against the original equation, coordinate rule and stated domain.
@why Subtract -1 from both sides: 3cos(u)=-1-(-1)=0. Divide both sides by the nonzero 3: cos(u)=(0)/(3)=0. Multiplication by 3 and addition of -1 reverse these operations.
@step 20 | Which range statement and circle-point count are correct for the isolated coordinate?
@choice 21 | 0\in[-1,1],\quad N=2
@choice 23 | 0\in[-1,1],\quad N=1
@choice 22 | 0\notin[-1,1],\quad N=0
@answer 21
@feedback 22 | 0 is in the coordinate range, including its endpoints. Declaring the equation impossible loses genuine solutions.
@feedback 23 | Since -1<0<1, the coordinate line cuts the circle twice, not once. Both matching points are needed.
@after 0\in[-1,1],\quad N=2
@wrong Check the current goal against the original equation, coordinate rule and stated domain.
@why The isolated coordinate 0 belongs to [-1,1]. Fixing it in x squared+y squared=1 leaves the other coordinate squared equal to 1. That is positive, with two distinct opposite roots and two circle points.
@step 30 | Which formula lists every real argument u at this coordinate, with n any integer?
@choice 32 | u=\frac{\pi}{2}+2n\pi,\quad n\in\mathbb{Z}
@choice 31 | u=\frac{\pi}{2}+2n\pi,\quad\text{or}\quad u=\frac{3\pi}{2}+2n\pi,\quad n\in\mathbb{Z}
@choice 33 | u=\frac{2\pi}{3}+2n\pi,\quad\text{or}\quad u=\frac{5\pi}{3}+2n\pi,\quad n\in\mathbb{Z}
@answer 31
@feedback 32 | This omits the valid argument 3pi/2. It has cos value 0. An empty formula or a single familiar branch is not complete.
@feedback 33 | Adding pi/6 to the true representatives changes their horizontal coordinates. For example, 2pi/3 does not have cos 0; adding full turns cannot repair that mismatch.
@after u=\frac{\pi}{2}+2n\pi,\quad\text{or}\quad u=\frac{3\pi}{2}+2n\pi,\quad n\in\mathbb{Z}
@wrong Check the current goal against the original equation, coordinate rule and stated domain.
@why The matching representatives in one argument turn are pi/2 and 3pi/2. Their cos values are 0, using the axis or special-triangle values and horizontal-axis reflection. There are exactly 2 distinct circle points at this coordinate. Cosine reflection sends pi/2 to -pi/2; adding 2pi gives 3pi/2. Adding 2n*pi to each gives every real argument, with n any integer and coincident reflected points counted once.
@step 40 | What interval for u is exactly equivalent to the original theta interval?
@choice 43 | -\frac{\pi}{2}\le u<\frac{5\pi}{2}
@choice 42 | 0\le u<\frac{3\pi}{2}
@choice 41 | -\frac{\pi}{2}\le u<\frac{3\pi}{2}
@answer 41
@feedback 42 | The lower endpoint must be 1(0)+(-pi/2)=-pi/2. This option shifts that endpoint by an extra pi/2 and discards part of the original theta domain.
@feedback 43 | The upper endpoint is 1(2pi)+(-pi/2)=3pi/2, still excluded. This option extends it by pi, admitting arguments from outside the requested theta interval.
@after -\frac{\pi}{2}\le u<\frac{3\pi}{2}
@wrong Check the current goal against the original equation, coordinate rule and stated domain.
@why Since k=1 is positive, multiplying 0<=theta<2pi by 1 preserves both inequality directions. Add -pi/2 to both bounds: the lower endpoint is -pi/2 and the upper is 3pi/2. The lower inequality stays closed; the upper stays strict.
@step 50 | Which complete argument set remains after applying the half-open interval?
@choice 51 | u\in\left\{-\frac{\pi}{2},\frac{\pi}{2}\right\}
@choice 52 | u\in\left\{\frac{\pi}{2}\right\}
@choice 53 | u\in\left\{-\frac{\pi}{2},\frac{\pi}{2},\frac{3\pi}{2}\right\}
@answer 51
@feedback 52 | This omits -pi/2, which has cos 0 and satisfies -pi/2<=u<3pi/2. The lower endpoint is included whenever it solves the equation.
@feedback 53 | This adds the excluded upper argument 3pi/2. It corresponds to theta=2pi, so it is forbidden even if it satisfies the trigonometric equation.
@after u\in\left\{-\frac{\pi}{2},\frac{\pi}{2}\right\}
@wrong Check the current goal against the original equation, coordinate rule and stated domain.
@why Filter the periodic formulas by -pi/2<=u<3pi/2: pi/2+2n*pi uses n=0; 3pi/2+2n*pi uses n=-1. This gives -pi/2, pi/2. No neighboring integer copy fits, because it crosses one of the endpoints. There is 1 complete argument turn, each with 2 matching points, so 2 distinct arguments exhaust the interval.
@step 60 | Which theta set results from undoing the complete argument transformation?
@choice 63 | \theta\in\left\{\frac{\pi}{12},\frac{13\pi}{12}\right\}
@choice 61 | \theta\in\left\{0,\pi\right\}
@choice 62 | \theta\in\left\{-\frac{\pi}{2},\frac{\pi}{2}\right\}
@answer 61
@feedback 62 | These are the u values, not the theta values. Use theta=(u-(-pi/2))/1; undo the phase shift instead of relabelling the argument.
@feedback 63 | Each listed theta is too large by pi/12. The required inverse is exactly theta=(u-(-pi/2))/1, with no additional offset.
@after \theta\in\left\{0,\pi\right\}
@wrong Check the current goal against the original equation, coordinate rule and stated domain.
@why u=-pi/2 gives theta=(-pi/2-(-pi/2))/1=0; u=pi/2 gives theta=(pi/2-(-pi/2))/1=pi. This invertible linear map preserves all 2 distinct solutions and sends the argument interval back to [0,2pi).
@step 70 | Which original substitution and solution count jointly verify the complete answer?
@choice 72 | (3)(0)+(-1)=0,\quad M=2
@choice 73 | (3)(0)+(-1)=-1,\quad M=3
@choice 71 | (3)(0)+(-1)=-1,\quad M=2
@answer 71
@feedback 72 | Evaluate the original left side carefully: (3)(0)+(-1)=-1, not 0. The isolated coordinate must reproduce the actual original right side.
@feedback 73 | The displayed substitution is valid, but the count is not. The transformed interval spans 1 full turn, giving 1 times 2=2 distinct angles. Counting one more repeats a point or admits a non-solution.
@after (3)(0)+(-1)=-1,\quad M=2
@wrong Check the current goal against the original equation, coordinate rule and stated domain.
@why Each retained theta gives one of the certified argument values -pi/2, pi/2, all with cos 0; hence (3)(0)+(-1)=-1. All 2 angles are in [0,2pi), with no duplicates. The coordinate-line count gives 2 points in the 1 argument turn, and the argument map is invertible. Therefore membership and completeness both hold.
@end

@question prod02_trig_full_equations_q09 | All real angles in a shifted sine equation
@template choices.v1
@version 1
@goal Find every real solution with an integer parameter and justify the complete result.
@given -2\sin\left(2\theta-\frac{\pi}{2}\right)+2=1,\quad\theta\in\mathbb{R}
@domain Theta is real and measured in radians. Find all real solutions, with n ranging over all integers. Write u=2theta-pi/2 for the argument. Alpha in a method denotes any known solution argument, not necessarily acute. N counts matching points in one argument turn; M counts branches in one theta period.
@read prod02_trig_full_equations_r
@step 10 | Which branch-construction rule works for sine at a known non-extreme, nonzero coordinate, with n an integer?
@choice 13 | u=\alpha+n\pi\ \text{or}\ u=\pi-\alpha+n\pi
@choice 11 | u=\alpha+2n\pi\ \text{or}\ u=\pi-\alpha+2n\pi
@choice 12 | u=\alpha+2n\pi\ \text{or}\ u=-\alpha+2n\pi
@answer 11
@feedback 12 | The other coordinate's reflection negates the required vertical coordinate. For nonzero coordinates this does not preserve the equation; use reflection across the vertical axis.
@feedback 13 | Adding n*pi includes odd half-turns, which negate a nonzero sine or cosine value. The argument branches repeat by 2n*pi, before division by 2.
@after u=\alpha+2n\pi\ \text{or}\ u=\pi-\alpha+2n\pi
@wrong Check the current goal against the original equation, coordinate rule and stated domain.
@why Sine is vertical height, preserved by alpha to pi-alpha. Each known branch repeats after a full argument turn, 2pi. The integer parameter supplies all such turns. We will find a numerical alpha by isolating and evaluating the original coordinate.
@step 20 | Which equation correctly isolates sine of the whole argument u?
@choice 22 | \sin u=1
@choice 21 | \sin u=\frac{1}{2}
@choice 23 | \sin u=0
@answer 21
@feedback 22 | This coordinate is too large by 1/2. It gives (-2)(1)+(2)=0, not 1. Subtract the entire constant and divide the entire right side.
@feedback 23 | This coordinate is too small by 1/2. It gives (-2)(0)+(2)=2, not 1. Keep the sign of the divisor -2.
@after u=2\theta-\frac{\pi}{2},\quad\sin u=\frac{1}{2}
@wrong Check the current goal against the original equation, coordinate rule and stated domain.
@why Subtract 2 from both sides: -2sin(u)=1-(2)=-1. Divide both sides by the nonzero -2: sin(u)=(-1)/(-2)=0.5. Multiplication by -2 and addition of 2 reverse these operations.
@step 30 | Which range statement and circle-point count are correct for the isolated coordinate?
@choice 33 | \frac{1}{2}\in[-1,1],\quad N=1
@choice 32 | \frac{1}{2}\notin[-1,1],\quad N=0
@choice 31 | \frac{1}{2}\in[-1,1],\quad N=2
@answer 31
@feedback 32 | 0.5 is in the coordinate range, including its endpoints. Declaring the equation impossible loses genuine solutions.
@feedback 33 | Since -1<0.5<1, the coordinate line cuts the circle twice, not once. Both matching points are needed.
@after \frac{1}{2}\in[-1,1],\quad N=2
@wrong Check the current goal against the original equation, coordinate rule and stated domain.
@why The isolated coordinate 0.5 belongs to [-1,1]. Fixing it in x squared+y squared=1 leaves the other coordinate squared equal to 0.75. That is positive, with two distinct opposite roots and two circle points.
@step 40 | Which formula lists every real argument u at this coordinate, with n any integer?
@choice 41 | u=\frac{\pi}{6}+2n\pi,\quad\text{or}\quad u=\frac{5\pi}{6}+2n\pi,\quad n\in\mathbb{Z}
@choice 42 | u=\frac{\pi}{6}+2n\pi,\quad n\in\mathbb{Z}
@choice 43 | u=\frac{\pi}{3}+2n\pi,\quad\text{or}\quad u=\pi+2n\pi,\quad n\in\mathbb{Z}
@answer 41
@feedback 42 | This omits the valid argument 5pi/6. It has sin value 0.5. An empty formula or a single familiar branch is not complete.
@feedback 43 | Adding pi/6 to the true representatives changes their vertical coordinates. For example, pi/3 does not have sin 0.5; adding full turns cannot repair that mismatch.
@after u=\frac{\pi}{6}+2n\pi,\quad\text{or}\quad u=\frac{5\pi}{6}+2n\pi,\quad n\in\mathbb{Z}
@wrong Check the current goal against the original equation, coordinate rule and stated domain.
@why The matching representatives in one argument turn are pi/6 and 5pi/6. Their sin values are 0.5, using the axis or special-triangle values and vertical-axis reflection. There are exactly 2 distinct circle points at this coordinate. The reflection calculation is pi-pi/6=5pi/6. Adding 2n*pi to each gives every real argument, with n any integer and coincident reflected points counted once.
@step 50 | Which general theta formula undoes both the phase and the frequency, including the integer term?
@choice 53 | \theta=\frac{\pi}{3}+2n\pi,\quad\text{or}\quad \theta=\frac{2\pi}{3}+2n\pi,\quad n\in\mathbb{Z}
@choice 51 | \theta=\frac{\pi}{3}+n\pi,\quad\text{or}\quad \theta=\frac{2\pi}{3}+n\pi,\quad n\in\mathbb{Z}
@choice 52 | \theta=\frac{\pi}{12}+n\pi,\quad\text{or}\quad \theta=\frac{5\pi}{12}+n\pi,\quad n\in\mathbb{Z}
@answer 51
@feedback 52 | This divides by 2 but omits subtraction of the phase -pi/2. The constant representatives must be (u-(-pi/2))/2, not u/2.
@feedback 53 | The constant representatives are correct, but the integer term must also be divided by 2. Using 2n*pi instead of n*pi misses every odd repeat of each theta branch.
@after \theta=\frac{\pi}{3}+n\pi,\quad\text{or}\quad \theta=\frac{2\pi}{3}+n\pi,\quad n\in\mathbb{Z}
@wrong Check the current goal against the original equation, coordinate rule and stated domain.
@why u=pi/6+2n*pi gives theta=(pi/6-(-pi/2))/2+(2/2)n*pi=pi/3+n*pi; u=5pi/6+2n*pi gives theta=(5pi/6-(-pi/2))/2+(2/2)n*pi=2pi/3+n*pi. Every integer n is allowed. The two residues are distinct modulo pi, so they do not duplicate each other.
@step 60 | Which substitution and branch count verify the result in each theta period pi?
@choice 62 | (-2)(\frac{1}{2})+(2)=2,\quad M=2
@choice 63 | (-2)(\frac{1}{2})+(2)=1,\quad M=3
@choice 61 | (-2)(\frac{1}{2})+(2)=1,\quad M=2
@answer 61
@feedback 62 | Evaluate the original left side carefully: (-2)(0.5)+(2)=1, not 2. The isolated coordinate must reproduce the actual original right side.
@feedback 63 | The displayed substitution is valid, but the count is not. There are 2 distinct residues modulo pi and two argument-coordinate points. Counting one more repeats a point or admits a non-solution.
@after (-2)(\frac{1}{2})+(2)=1,\quad M=2
@wrong Check the current goal against the original equation, coordinate rule and stated domain.
@why For the first branch, u=2(pi/3+n*pi)-pi/2=pi/6+2n*pi. For the second, u=2(2pi/3+n*pi)-pi/2=5pi/6+2n*pi. Thus for every integer n, sin(u)=0.5 exactly and (-2)(0.5)+(2)=1. The inverse argument map carries every periodic argument branch to one of these 2 theta branches. This proves both substitution for all integers and completeness, not just a few examples.
@end

@question prod02_trig_full_equations_q10 | All real angles in a shifted cosine equation
@template choices.v1
@version 1
@goal Find every real solution with an integer parameter and justify the complete result.
@given 2\cos\left(2\theta+\frac{\pi}{2}\right)+1=0,\quad\theta\in\mathbb{R}
@domain Theta is real and measured in radians. Find all real solutions, with n ranging over all integers. Write u=2theta+pi/2 for the argument. Alpha in a method denotes any known solution argument, not necessarily acute. N counts matching points in one argument turn; M counts branches in one theta period.
@read prod02_trig_full_equations_r
@step 10 | Which branch-construction rule works for cosine at a known non-extreme, nonzero coordinate, with n an integer?
@choice 11 | u=\alpha+2n\pi\ \text{or}\ u=-\alpha+2n\pi
@choice 12 | u=\alpha+2n\pi\ \text{or}\ u=\pi-\alpha+2n\pi
@choice 13 | u=\alpha+n\pi\ \text{or}\ u=-\alpha+n\pi
@answer 11
@feedback 12 | The other coordinate's reflection negates the required horizontal coordinate. For nonzero coordinates this does not preserve the equation; use reflection across the horizontal axis.
@feedback 13 | Adding n*pi includes odd half-turns, which negate a nonzero sine or cosine value. The argument branches repeat by 2n*pi, before division by 2.
@after u=\alpha+2n\pi\ \text{or}\ u=-\alpha+2n\pi
@wrong Check the current goal against the original equation, coordinate rule and stated domain.
@why Cosine is horizontal coordinate, preserved by alpha to -alpha. Each known branch repeats after a full argument turn, 2pi. The integer parameter supplies all such turns. We will find a numerical alpha by isolating and evaluating the original coordinate.
@step 20 | Which equation correctly isolates cosine of the whole argument u?
@choice 23 | \cos u=-1
@choice 22 | \cos u=0
@choice 21 | \cos u=-\frac{1}{2}
@answer 21
@feedback 22 | This coordinate is too large by 1/2. It gives (2)(0)+(1)=1, not 0. Subtract the entire constant and divide the entire right side.
@feedback 23 | This coordinate is too small by 1/2. It gives (2)(-1)+(1)=-1, not 0. Keep the sign of the divisor 2.
@after u=2\theta+\frac{\pi}{2},\quad\cos u=-\frac{1}{2}
@wrong Check the current goal against the original equation, coordinate rule and stated domain.
@why Subtract 1 from both sides: 2cos(u)=0-(1)=-1. Divide both sides by the nonzero 2: cos(u)=(-1)/(2)=-0.5. Multiplication by 2 and addition of 1 reverse these operations.
@step 30 | Which range statement and circle-point count are correct for the isolated coordinate?
@choice 31 | -\frac{1}{2}\in[-1,1],\quad N=2
@choice 32 | -\frac{1}{2}\notin[-1,1],\quad N=0
@choice 33 | -\frac{1}{2}\in[-1,1],\quad N=1
@answer 31
@feedback 32 | -0.5 is in the coordinate range, including its endpoints. Declaring the equation impossible loses genuine solutions.
@feedback 33 | Since -1<-0.5<1, the coordinate line cuts the circle twice, not once. Both matching points are needed.
@after -\frac{1}{2}\in[-1,1],\quad N=2
@wrong Check the current goal against the original equation, coordinate rule and stated domain.
@why The isolated coordinate -0.5 belongs to [-1,1]. Fixing it in x squared+y squared=1 leaves the other coordinate squared equal to 0.75. That is positive, with two distinct opposite roots and two circle points.
@step 40 | Which formula lists every real argument u at this coordinate, with n any integer?
@choice 43 | u=\frac{5\pi}{6}+2n\pi,\quad\text{or}\quad u=\frac{3\pi}{2}+2n\pi,\quad n\in\mathbb{Z}
@choice 41 | u=\frac{2\pi}{3}+2n\pi,\quad\text{or}\quad u=\frac{4\pi}{3}+2n\pi,\quad n\in\mathbb{Z}
@choice 42 | u=\frac{2\pi}{3}+2n\pi,\quad n\in\mathbb{Z}
@answer 41
@feedback 42 | This omits the valid argument 4pi/3. It has cos value -0.5. An empty formula or a single familiar branch is not complete.
@feedback 43 | Adding pi/6 to the true representatives changes their horizontal coordinates. For example, 5pi/6 does not have cos -0.5; adding full turns cannot repair that mismatch.
@after u=\frac{2\pi}{3}+2n\pi,\quad\text{or}\quad u=\frac{4\pi}{3}+2n\pi,\quad n\in\mathbb{Z}
@wrong Check the current goal against the original equation, coordinate rule and stated domain.
@why The matching representatives in one argument turn are 2pi/3 and 4pi/3. Their cos values are -0.5, using the axis or special-triangle values and horizontal-axis reflection. There are exactly 2 distinct circle points at this coordinate. Cosine reflection gives -2pi/3; adding 2pi gives 4pi/3. Adding 2n*pi to each gives every real argument, with n any integer and coincident reflected points counted once.
@step 50 | Which general theta formula undoes both the phase and the frequency, including the integer term?
@choice 52 | \theta=\frac{\pi}{3}+n\pi,\quad\text{or}\quad \theta=\frac{2\pi}{3}+n\pi,\quad n\in\mathbb{Z}
@choice 53 | \theta=\frac{\pi}{12}+2n\pi,\quad\text{or}\quad \theta=\frac{5\pi}{12}+2n\pi,\quad n\in\mathbb{Z}
@choice 51 | \theta=\frac{\pi}{12}+n\pi,\quad\text{or}\quad \theta=\frac{5\pi}{12}+n\pi,\quad n\in\mathbb{Z}
@answer 51
@feedback 52 | This divides by 2 but omits subtraction of the phase pi/2. The constant representatives must be (u-(pi/2))/2, not u/2.
@feedback 53 | The constant representatives are correct, but the integer term must also be divided by 2. Using 2n*pi instead of n*pi misses every odd repeat of each theta branch.
@after \theta=\frac{\pi}{12}+n\pi,\quad\text{or}\quad \theta=\frac{5\pi}{12}+n\pi,\quad n\in\mathbb{Z}
@wrong Check the current goal against the original equation, coordinate rule and stated domain.
@why u=2pi/3+2n*pi gives theta=(2pi/3-(pi/2))/2+(2/2)n*pi=pi/12+n*pi; u=4pi/3+2n*pi gives theta=(4pi/3-(pi/2))/2+(2/2)n*pi=5pi/12+n*pi. Every integer n is allowed. The two residues are distinct modulo pi, so they do not duplicate each other.
@step 60 | Which substitution and branch count verify the result in each theta period pi?
@choice 61 | (2)(-\frac{1}{2})+(1)=0,\quad M=2
@choice 63 | (2)(-\frac{1}{2})+(1)=0,\quad M=3
@choice 62 | (2)(-\frac{1}{2})+(1)=1,\quad M=2
@answer 61
@feedback 62 | Evaluate the original left side carefully: (2)(-0.5)+(1)=0, not 1. The isolated coordinate must reproduce the actual original right side.
@feedback 63 | The displayed substitution is valid, but the count is not. There are 2 distinct residues modulo pi and two argument-coordinate points. Counting one more repeats a point or admits a non-solution.
@after (2)(-\frac{1}{2})+(1)=0,\quad M=2
@wrong Check the current goal against the original equation, coordinate rule and stated domain.
@why For the first branch, u=2(pi/12+n*pi)+pi/2=2pi/3+2n*pi. For the second, u=2(5pi/12+n*pi)+pi/2=4pi/3+2n*pi. Thus for every integer n, cos(u)=-0.5 exactly and (2)(-0.5)+(1)=0. The inverse argument map carries every periodic argument branch to one of these 2 theta branches. This proves both substitution for all integers and completeness, not just a few examples.
@end

@question prod02_trig_full_equations_q11 | A full sine equation
@template choices.v1
@version 1
@goal Find every solution in the stated interval and check the original equation and completeness.
@given 1\sin\left(2\theta\right)-1=1,\quad0\le\theta<2\pi
@domain Theta is real and measured in radians. Find every solution in [0,2pi). Write u=2theta for the argument. N counts matching points in one argument turn; M counts solutions in the requested theta interval.
@read prod02_trig_full_equations_r
@step 10 | Which equation correctly isolates sine of the whole argument u?
@choice 13 | \sin u=0
@choice 11 | \sin u=2
@choice 12 | \sin u=1
@answer 11
@feedback 12 | This clips the computed coordinate to a range endpoint. But (1)(1)+(-1)=0, not the original right side 1. Compute (1-(-1))/1 exactly before deciding whether the result is possible.
@feedback 13 | Zero would require the original constant B to equal C. Here B=-1 and C=1, so (1)(0)+(-1)=-1 does not check the equation.
@after u=2\theta,\quad\sin u=2
@wrong Check the current goal against the original equation, coordinate rule and stated domain.
@why Subtract -1 from both sides: 1sin(u)=1-(-1)=2. Divide both sides by the nonzero 1: sin(u)=(2)/(1)=2. Multiplication by 1 and addition of -1 reverse these operations.
@step 20 | Which range statement and circle-point count are correct for the isolated coordinate?
@choice 21 | 2>1,\quad N=0
@choice 22 | 2\in[-1,1],\quad N=2
@choice 23 | 2\in[-1,1],\quad N=1
@answer 21
@feedback 22 | 2 lies outside [-1,1]; no unit-circle point has that coordinate, so there cannot be two such points.
@feedback 23 | One circle point occurs at coordinate 1 or -1, not at 2. The isolated coordinate is outside the permitted range.
@after 2>1,\quad N=0
@wrong Check the current goal against the original equation, coordinate rule and stated domain.
@why The exact coordinate is 2, which is greater than 1. Sine and cosine are coordinates of a radius-one circle, so neither can take that value for any real argument. The shift and frequency cannot change that range.
@step 30 | Which complete solution set follows from the range contradiction?
@choice 33 | S=\left\{0\right\}
@choice 31 | S=\varnothing
@choice 32 | S=\mathbb{R}
@answer 31
@feedback 32 | Every real theta would require sin(u)=2, but that coordinate is impossible. The equation is a contradiction, not an identity.
@feedback 33 | Theta=0 does not overcome the range contradiction. Its argument is 0, and no argument has sin value 2; selecting one angle cannot solve the equation.
@after S=\varnothing
@wrong Check the current goal against the original equation, coordinate rule and stated domain.
@why If any real theta solved the original, subtracting -1 and dividing by 1 would force sin(2theta)=2. This contradicts the unit-circle coordinate range. Thus no theta solves the original equality, and the requested interval contains no solutions either. The empty set is complete because existence itself is impossible.
@end

@question prod02_trig_full_equations_q12 | A full cosine equation
@template choices.v1
@version 1
@goal Find every solution in the stated interval and check the original equation and completeness.
@given -1\cos\left(\theta+\frac{\pi}{2}\right)+1=3,\quad0\le\theta<2\pi
@domain Theta is real and measured in radians. Find every solution in [0,2pi). Write u=theta+pi/2 for the argument. N counts matching points in one argument turn; M counts solutions in the requested theta interval.
@read prod02_trig_full_equations_r
@step 10 | Which equation correctly isolates cosine of the whole argument u?
@choice 12 | \cos u=-1
@choice 13 | \cos u=0
@choice 11 | \cos u=-2
@answer 11
@feedback 12 | This clips the computed coordinate to a range endpoint. But (-1)(-1)+(1)=2, not the original right side 3. Compute (3-1)/(-1) exactly before checking its range.
@feedback 13 | Zero would require B=C. Here B=1 and C=3, so (-1)(0)+(1)=1 does not check the original equation.
@after u=\theta+\frac{\pi}{2},\quad\cos u=-2
@wrong Check the current goal against the original equation, coordinate rule and stated domain.
@why Subtract 1 from both sides: -1cos(u)=3-(1)=2. Divide both sides by the nonzero -1: cos(u)=(2)/(-1)=-2. Multiplication by -1 and addition of 1 reverse these operations.
@step 20 | Which range statement and circle-point count are correct for the isolated coordinate?
@choice 23 | -2\in[-1,1],\quad N=1
@choice 21 | -2<-1,\quad N=0
@choice 22 | -2\in[-1,1],\quad N=2
@answer 21
@feedback 22 | -2 lies outside [-1,1]; no unit-circle point has that coordinate, so there cannot be two such points.
@feedback 23 | One circle point occurs at coordinate 1 or -1, not at -2. The isolated coordinate is outside the permitted range.
@after -2<-1,\quad N=0
@wrong Check the current goal against the original equation, coordinate rule and stated domain.
@why The exact coordinate is -2, which is less than -1. Sine and cosine are coordinates of a radius-one circle, so neither can take that value for any real argument. The shift and frequency cannot change that range.
@step 30 | Which complete solution set follows from the range contradiction?
@choice 32 | S=\mathbb{R}
@choice 33 | S=\left\{0\right\}
@choice 31 | S=\varnothing
@answer 31
@feedback 32 | Every real theta would require cos(u)=-2, but that coordinate is impossible. The equation is a contradiction, not an identity.
@feedback 33 | Theta=0 does not overcome the range contradiction. Its argument is pi/2, and no argument has cos value -2; selecting one angle cannot solve the equation.
@after S=\varnothing
@wrong Check the current goal against the original equation, coordinate rule and stated domain.
@why If any real theta solved the original, subtracting 1 and dividing by -1 would force cos(theta+pi/2)=-2. This contradicts the unit-circle coordinate range. Thus no theta solves the original equality, and the requested interval contains no solutions either. The empty set is complete because existence itself is impossible.
@end
