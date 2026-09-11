@paths 1
@subject trigonometry | Trigonometry
@chapter topic_0033 | Identities

@lesson prod04_trig_identity_equations_r | Solving equations with identities
@template lesson.v2
@block introduction | start | 4.1 | From identities to complete answers
@prose An identity is an equality true everywhere on its common domain; an equation asks which inputs make a particular equality true. We use identities to rewrite equations without changing their solutions, then use algebra to find all branches. Finding one familiar angle is not a proof that the solution set is complete.
@endblock
@block definition | terms | 4.2 | Coordinates, factors and solution sets
@prose Theta is a real angle in radians. On the unit circle, s=sin(theta) is the vertical coordinate and c=cos(theta) is the horizontal coordinate. A half-turn is pi radians. The interval [0,2pi) includes 0 and excludes 2pi. S denotes the complete set of solutions in that interval; braces denote a set without order or repetition, and the empty-set symbol means no solutions. D is the original equation's domain.
@display coordinates
s=\sin\theta,\qquad c=\cos\theta,\qquad s^2+c^2=1,\qquad -1\le s,c\le1
@prose A factor is multiplied by another factor; a term is added or subtracted. A quadratic in sine is a quadratic polynomial whose variable is the sine value. Substitution means temporarily naming such a value by another letter: for example u=sin(theta). Solving for u does not yet solve for theta. The zero-product rule says that a product of real factors equals zero exactly when at least one factor is zero. The word or means take the union of all such branches, not just the first branch found.
@prose N counts the distinct retained angles. R counts retained angles that fail the original equation. A finished nonempty solution set has R=0. An empty solution set also has R=0, but requires a separate impossibility proof rather than a substitution claim.
@endblock
@block proposition | rule | 4.3 | Reversible identities and algebra
@prose The circle relation gives sin squared=1-cos squared and cos squared=1-sin squared. Use the full replacement inside its original coefficient. Reciprocal and quotient definitions are identities only where their denominators are nonzero. Cotangent is cosine divided by sine; it remains defined at cosine zeros when sine is nonzero.
@display definitions
\sec\theta=\frac{1}{c},\quad\tan\theta=\frac{s}{c}\quad(c\ne0),\qquad
\csc\theta=\frac{1}{s},\quad\cot\theta=\frac{c}{s}\quad(s\ne0)
@prose Subtracting the same expression from both sides preserves an equation. Multiplication or division by a nonzero constant is reversible. Multiplication by a variable denominator is reversible only on its original nonzero domain. For a difference of squares, a squared-b squared=(a-b)(a+b). Expanding the proposed factors is an exact check of a factorization; matching them at a few angles is not.
@display zero_product
AB=0\quad\Longleftrightarrow\quad A=0\ \text{or}\ B=0
@prose Do not divide by sine or cosine merely because it is a common factor. If that coordinate can be zero, division loses a whole branch. Move all terms to one side and factor instead, or explicitly handle the zero case before division.
@help proof
@prose If M is nonzero on D, the original equation L=H is equivalent there to M(L-H)=0. If exact expansion and the circle relation prove M(L-H)=AB, then zero product proves that A=0 or B=0 gives all possible solutions. This is a reversible proof on D, not a guess from a finite list of angles.
@endblock
@block proposition | condition | 4.4 | From algebraic roots to all angles
@prose Sine and cosine values must lie in [-1,1]. A polynomial root outside that range produces no angle. Reciprocal equations also retain original exclusions after clearing denominators: secant and tangent exclude cosine zero; cosecant and cotangent exclude sine zero. The number 2pi is forbidden by this family's interval even when it is coterminal with a valid zero angle.
@prose An inverse trigonometric function returns one principal value, not the entire solution set. Inverse sine returns a value in [-pi/2,pi/2], and inverse cosine in [0,pi]. Sine reflection sends an angle alpha to pi-alpha; cosine reflection sends it to -alpha. Add a full turn when necessary to put a representative in [0,2pi). Sine and cosine have period 2pi, meaning their values repeat after a full turn.
@display reflections
\sin(\pi-\alpha)=\sin\alpha,\qquad\cos(-\alpha)=\cos\alpha
@prose Fixing one coordinate at h leaves the other coordinate squared equal to 1-h squared. Thus a non-extreme value in (-1,1) gives two unit-circle points, an extreme value gives one, and a value outside [-1,1] gives none. This proves the number of angles per branch. Unite all branch sets and remove overlaps; do not add branch counts when they share a point.
@prose The half-coordinate angles follow from bisecting an equilateral triangle of side 2: its right-triangle sides are 1, sqrt(3) and 2. Hence sin(pi/6)=1/2 and cos(pi/3)=1/2. Reflections and quadrant signs give the other half-coordinate points. Axis coordinates give zero and extreme values. Check each proposed representative exactly, and use the intersection count to prove that no others remain.
@endblock
@block example | worked | 4.5 | One equation with overlapping branches
@prose Find every solution in the stated interval, retaining the original domain and checking the original equality.
@display worked_given
\tan\theta=\sin\theta,\qquad0\le\theta<2\pi
@help hint
@prose Rewrite tangent as a coordinate quotient. Clear its nonzero denominator, then factor rather than dividing by sine. Check whether the resulting coordinate branches overlap before counting angles.
@help answer
@display
S=\left\{0,\pi\right\},\qquad D:\ c\ne0,\qquad N=2,\quad R=0
@help solution
@prose The original tangent requires c nonzero, so pi/2 and 3pi/2 are excluded. Write tan(theta)=s/c and sin(theta)=s. Multiply s/c=s by nonzero c to get s=sc. Move the right side left and factor: s-sc=s(1-c)=0.
@display worked_factor
\frac{s}{c}=s\quad\Longleftrightarrow\quad s-sc=0\quad\Longleftrightarrow\quad s(1-c)=0,\qquad c\ne0
@prose Zero product gives s=0 or c=1. On [0,2pi), sine zero gives 0 and pi. Cosine 1 gives only 0, already in the sine-zero branch. The union therefore has two distinct angles, not three. Both have nonzero cosine; the upper endpoint 2pi is excluded.
@display worked_union
S=\left\{0,\pi\right\}\cup\left\{0\right\}=\left\{0,\pi\right\}
@prose At theta=0, sine is 0 and cosine is 1, so tangent is 0/1=0. At theta=pi, sine is 0 and cosine is -1, so tangent is 0/(-1)=0. Thus both original equalities hold. The nonzero clearing multiplier, complete zero-product branches and unit-circle counts prove there are no other permitted angles.
@endblock
@block example | errors | 4.6 | Five ways to lose the original problem
@prose Dividing by a possibly zero factor can discard roots. Taking only the positive root of a square or only a principal inverse value can omit branches. Keeping an algebraic coordinate outside [-1,1] invents an impossible angle. Forgetting a reciprocal exclusion can restore an undefined input. Counting 0 and 2pi together violates the half-open interval and repeats a circle direction.
@prose Truth and requested form are also distinct. An expanded polynomial and its correct factorization define the same zero equation, but an expanded expression does not answer a request to factor. A partial branch list may contain valid roots while still failing the goal of listing every algebraic branch. Explain the missed goal accurately instead of calling a valid intermediate calculation false.
@endblock
@block exercise | practice | 4.7 | Twelve complete routes
@prose Start with four factored or squared-coordinate equations. Then use identities and reciprocals, including a coordinate-range contradiction. Finish with four mixed equations that select a strategy, preserve zero factors and respect original domains. Each route ends with an original-equation check and a proof that its angle set is complete.
@endblock
@block summary | summary | 4.8 | A complete solve and its source
@prose Determine D first. Use a reversible identity or nonzero multiplier, factor and retain every zero-product branch, apply coordinate ranges, construct complete interval angle sets, remove overlaps and check the original equation. A few successful substitutions establish membership, not completeness.
@prose Adapted from David Lippman and Melonie Rasmussen, Precalculus: An Investigation of Functions, Edition 2.3, Section 7.1, Examples 1-4 and selected Exercises 13-23 and 40. Source: https://www.opentextbookstore.com/precalc/2.3/Chapter%207.pdf . The lesson and questions are adapted content under Creative Commons Attribution-ShareAlike 4.0 International: https://creativecommons.org/licenses/by-sa/4.0/ . Changes include selected coefficients, explicit interval/domain conditions and new teaching, choices and feedback. Exact seed locators and changes accompany the authoring metadata. No author endorsement is implied.
@endblock
@practice prod04_trig_identity_equations_q01
@practice prod04_trig_identity_equations_q02
@practice prod04_trig_identity_equations_q03
@practice prod04_trig_identity_equations_q04
@practice prod04_trig_identity_equations_q05
@practice prod04_trig_identity_equations_q06
@practice prod04_trig_identity_equations_q07
@practice prod04_trig_identity_equations_q08
@practice prod04_trig_identity_equations_q09
@practice prod04_trig_identity_equations_q10
@practice prod04_trig_identity_equations_q11
@practice prod04_trig_identity_equations_q12
@end

@question prod04_trig_identity_equations_q01 | A sine polynomial
@template choices.v1
@version 1
@goal Find every solution of the original equation in [0,2pi). Retain its original domain, justify every branch and check the complete set.
@given \left(2\,{s}^{2}+s\right)=0,\quad 0\le\theta<2\pi
@domain Theta is real and measured in radians. The interval includes 0 and excludes 2pi. Write s=sin(theta), c=cos(theta); D is the original domain and S the complete angle set. N counts retained solutions and R counts those failing the original equation.
@read prod04_trig_identity_equations_r
@step 10 | Within the stated interval, which additional condition gives the full original domain?
@choice 11 | \theta\in\mathbb{R}
@choice 12 | s\ne0
@choice 13 | c\ne0
@answer 11
@feedback 12 | There is no division by sine in the original equation. Imposing s nonzero would exclude inputs where the original is defined.
@feedback 13 | There is no division by cosine in the original equation. Cosine-zero inputs remain part of the original domain.
@after D:\theta\in\mathbb{R},\quad 0\le\theta<2\pi
@wrong Check the requested form, every branch and the original domain before proceeding.
@why The original equation contains only sine and cosine, both defined for every real angle. There is no variable denominator, so the interval is the only restriction.
@step 20 | Factor the zero-side polynomial as a product of two linear coordinate factors.
@choice 22 | s\,\left(2\,s-1\right)=0
@choice 23 | s\,\left(s+1\right)=0
@choice 21 | s\,\left(2\,s+1\right)=0
@answer 21
@feedback 22 | The product s(2s-1) expands to 2s squared-s, reversing the linear sign.
@feedback 23 | The product s(s+1) expands to s squared+s; it loses the coefficient 2.
@after s\,\left(2\,s+1\right)=0,\quad D:\theta\in\mathbb{R},\quad 0\le\theta<2\pi
@wrong Check the requested form, every branch and the original domain before proceeding.
@why Both terms share one s: 2s squared+s=s(2s+1). Expanding the product returns 2s squared+s. No division by s has occurred, so its zero branch is retained.
@step 30 | Use zero product to list every algebraic coordinate branch before range filtering.
@choice 32 | s=-\frac{1}{2}
@choice 31 | s=0,\quad\text{or}\quad s=-\frac{1}{2}
@choice 33 | s=0,\quad\text{or}\quad s=\frac{1}{2}
@answer 31
@feedback 32 | Dividing by s would discard the valid factor s=0. Keep both branches.
@feedback 33 | The second equation gives 2s=-1, hence s=-1/2 rather than +1/2.
@after s=0,\quad\text{or}\quad s=-\frac{1}{2},\quad D:\theta\in\mathbb{R},\quad 0\le\theta<2\pi
@wrong Check the requested form, every branch and the original domain before proceeding.
@why A product is zero exactly when at least one factor is zero. The first gives s=0. From 2s+1=0, subtract 1 and divide by 2 to obtain s=-1/2. Both are in [-1,1].
@step 40 | Which set contains every permitted angle in [0,2pi), with no duplicates?
@choice 41 | S=\left\{0,\pi,\frac{7\pi}{6},\frac{11\pi}{6}\right\}
@choice 42 | S=\left\{\pi,\frac{7\pi}{6},\frac{11\pi}{6}\right\}
@choice 43 | S=\left\{0,\pi,\frac{7\pi}{6},\frac{11\pi}{6},\frac{\pi}{2}\right\}
@answer 41
@feedback 42 | This omits theta=0. It is the included lower endpoint, has s=0, and gives 2(0 squared)+0=0 in the original equation. Keep this zero-factor root.
@feedback 43 | The extra pi/2 has s=1, so the original left side is 2(1 squared)+1=3, not 0. It is not a solution.
@after S=\left\{0,\pi,\frac{7\pi}{6},\frac{11\pi}{6}\right\},\quad D:\theta\in\mathbb{R},\quad 0\le\theta<2\pi
@wrong Check the requested form, every branch and the original domain before proceeding.
@why Sine zero gives 0 and pi on this interval. Sine -1/2 gives 7pi/6 and 11pi/6 by vertical-axis reflection. Each non-extreme coordinate has exactly two circle points. The four points are distinct; retain 0 and omit its coterminal upper endpoint 2pi.
@step 50 | Which solution count N and original-equation failure count R certify the complete set?
@choice 52 | N=5,\quad R=0
@choice 53 | N=4,\quad R=1
@choice 51 | N=4,\quad R=0
@answer 51
@feedback 52 | There are two points for s=0 and two for s=-1/2, giving four, not five. An extra point would repeat an endpoint or fail the original.
@feedback 53 | Each retained angle satisfies the original equality exactly; no retained angle fails. The failure count R is zero, not one.
@after S=\left\{0,\pi,\frac{7\pi}{6},\frac{11\pi}{6}\right\},\quad N=4,\quad R=0,\quad D:\theta\in\mathbb{R},\quad 0\le\theta<2\pi
@wrong Check the requested form, every branch and the original domain before proceeding.
@why At s=0, 2s squared+s=0. At s=-1/2, the same original expression is 2(1/4)-1/2=0. Each coordinate has two unit-circle points and the branches do not overlap, so all four angles satisfy the original, and zero product proves there are no others.
@end

@question prod04_trig_identity_equations_q02 | A second sine polynomial
@template choices.v1
@version 1
@goal Find every solution of the original equation in [0,2pi). Retain its original domain, justify every branch and check the complete set.
@given \left(\left(2\,{s}^{2}+3\,s\right)+1\right)=0,\quad 0\le\theta<2\pi
@domain Theta is real and measured in radians. The interval includes 0 and excludes 2pi. Write s=sin(theta), c=cos(theta); D is the original domain and S the complete angle set. N counts retained solutions and R counts those failing the original equation.
@read prod04_trig_identity_equations_r
@step 10 | Within the stated interval, which additional condition gives the full original domain?
@choice 12 | s\ne0
@choice 11 | \theta\in\mathbb{R}
@choice 13 | c\ne0
@answer 11
@feedback 12 | There is no division by sine in the original equation. Imposing s nonzero would exclude inputs where the original is defined.
@feedback 13 | There is no division by cosine in the original equation. Cosine-zero inputs remain part of the original domain.
@after D:\theta\in\mathbb{R},\quad 0\le\theta<2\pi
@wrong Check the requested form, every branch and the original domain before proceeding.
@why The original equation contains only sine and cosine, both defined for every real angle. There is no variable denominator, so the interval is the only restriction.
@step 20 | Factor the zero-side polynomial as a product of two linear coordinate factors.
@choice 21 | \left(2\,s+1\right)\,\left(s+1\right)=0
@choice 22 | \left(2\,s+1\right)\,\left(s-1\right)=0
@choice 23 | \left(2\,s-1\right)\,\left(s-1\right)=0
@answer 21
@feedback 22 | Changing the second factor to s-1 gives 2s squared-s-1, not the original polynomial.
@feedback 23 | Two minus signs give 2s squared-3s+1; the original linear coefficient is positive 3.
@after \left(2\,s+1\right)\,\left(s+1\right)=0,\quad D:\theta\in\mathbb{R},\quad 0\le\theta<2\pi
@wrong Check the requested form, every branch and the original domain before proceeding.
@why The factors (2s+1)(s+1) give 2s squared+2s+s+1=2s squared+3s+1. The two middle terms total 3s.
@step 30 | Use zero product to list every algebraic coordinate branch before range filtering.
@choice 32 | s=\frac{1}{2},\quad\text{or}\quad s=1
@choice 33 | s=-\frac{1}{2}
@choice 31 | s=-\frac{1}{2},\quad\text{or}\quad s=-1
@answer 31
@feedback 32 | Moving each +1 across the equality gives a negative right side, so both roots are negative.
@feedback 33 | The second factor supplies the distinct valid root s=-1. It cannot be dropped.
@after s=-\frac{1}{2},\quad\text{or}\quad s=-1,\quad D:\theta\in\mathbb{R},\quad 0\le\theta<2\pi
@wrong Check the requested form, every branch and the original domain before proceeding.
@why Solve each factor: 2s+1=0 gives s=-1/2, and s+1=0 gives s=-1. The value -1 is allowed; it is an extreme coordinate, not an impossible one.
@step 40 | Which set contains every permitted angle in [0,2pi), with no duplicates?
@choice 42 | S=\left\{\frac{3\pi}{2},\frac{11\pi}{6}\right\}
@choice 41 | S=\left\{\frac{7\pi}{6},\frac{3\pi}{2},\frac{11\pi}{6}\right\}
@choice 43 | S=\left\{\frac{7\pi}{6},\frac{3\pi}{2},\frac{11\pi}{6},0\right\}
@answer 41
@feedback 42 | This omits 7pi/6, where s=-1/2. The original gives 2(1/4)+3(-1/2)+1=1/2-3/2+1=0, so that reflected-branch angle must remain.
@feedback 43 | The extra theta=0 has s=0, so the original gives 2(0 squared)+3(0)+1=1, not 0. Being an included endpoint does not make it a solution.
@after S=\left\{\frac{7\pi}{6},\frac{3\pi}{2},\frac{11\pi}{6}\right\},\quad D:\theta\in\mathbb{R},\quad 0\le\theta<2\pi
@wrong Check the requested form, every branch and the original domain before proceeding.
@why Sine -1/2 occurs at 7pi/6 and 11pi/6. Sine -1 occurs only at the bottom point 3pi/2; reflection does not create a second point at an extreme. These three distinct angles exhaust the two algebraic branches.
@step 50 | Which solution count N and original-equation failure count R certify the complete set?
@choice 51 | N=3,\quad R=0
@choice 52 | N=4,\quad R=0
@choice 53 | N=3,\quad R=1
@answer 51
@feedback 52 | The extreme sine value -1 contributes one point, not two. The total is two plus one, which is three.
@feedback 53 | Each retained angle satisfies the original equality exactly; no retained angle fails. The failure count R is zero, not one.
@after S=\left\{\frac{7\pi}{6},\frac{3\pi}{2},\frac{11\pi}{6}\right\},\quad N=3,\quad R=0,\quad D:\theta\in\mathbb{R},\quad 0\le\theta<2\pi
@wrong Check the requested form, every branch and the original domain before proceeding.
@why For s=-1/2, the original gives 2(1/4)+3(-1/2)+1=1/2-3/2+1=0. For s=-1, it gives 2-3+1=0. The first coordinate contributes two points and the extreme contributes one, proving exactly three solutions.
@end

@question prod04_trig_identity_equations_q03 | A squared sine value
@template choices.v1
@version 1
@goal Find every solution of the original equation in [0,2pi). Retain its original domain, justify every branch and check the complete set.
@given {s}^{2}=\frac{1}{4},\quad 0\le\theta<2\pi
@domain Theta is real and measured in radians. The interval includes 0 and excludes 2pi. Write s=sin(theta), c=cos(theta); D is the original domain and S the complete angle set. N counts retained solutions and R counts those failing the original equation.
@read prod04_trig_identity_equations_r
@step 10 | Within the stated interval, which additional condition gives the full original domain?
@choice 12 | s\ne0
@choice 13 | c\ne0
@choice 11 | \theta\in\mathbb{R}
@answer 11
@feedback 12 | There is no division by sine in the original equation. Imposing s nonzero would exclude inputs where the original is defined.
@feedback 13 | There is no division by cosine in the original equation. Cosine-zero inputs remain part of the original domain.
@after D:\theta\in\mathbb{R},\quad 0\le\theta<2\pi
@wrong Check the requested form, every branch and the original domain before proceeding.
@why The original equation contains only sine and cosine, both defined for every real angle. There is no variable denominator, so the interval is the only restriction.
@step 20 | Clear the constant denominator and put zero on the right.
@choice 22 | \left(4\,{s}^{2}-4\right)=0
@choice 21 | \left(4\,{s}^{2}-1\right)=0
@choice 23 | \left(4\,{s}^{2}+1\right)=0
@answer 21
@feedback 22 | The right side becomes 4 times 1/4=1, not 4.
@feedback 23 | Moving the right-side 1 to the left subtracts it; it does not add 1.
@after \left(4\,{s}^{2}-1\right)=0,\quad D:\theta\in\mathbb{R},\quad 0\le\theta<2\pi
@wrong Check the requested form, every branch and the original domain before proceeding.
@why Multiply both sides of s squared=1/4 by nonzero 4 to obtain 4s squared=1. Subtract 1 to get 4s squared-1=0. This is reversible and does not restrict the domain.
@step 30 | Factor the zero-side polynomial as a product of two linear coordinate factors.
@choice 31 | \left(2\,s-1\right)\,\left(2\,s+1\right)=0
@choice 32 | \left(2\,s-1\right)\,\left(2\,s-1\right)=0
@choice 33 | \left(2\,s+1\right)\,\left(2\,s+1\right)=0
@answer 31
@feedback 32 | Squaring 2s-1 introduces a -4s term absent from the original.
@feedback 33 | Squaring 2s+1 introduces a +4s term absent from the original.
@after \left(2\,s-1\right)\,\left(2\,s+1\right)=0,\quad D:\theta\in\mathbb{R},\quad 0\le\theta<2\pi
@wrong Check the requested form, every branch and the original domain before proceeding.
@why Use difference of squares: 4s squared-1=(2s-1)(2s+1). The opposite middle terms cancel.
@step 40 | Use zero product to list every algebraic coordinate branch before range filtering.
@choice 42 | s=\frac{1}{2}
@choice 43 | s=-1,\quad\text{or}\quad s=1
@choice 41 | s=-\frac{1}{2},\quad\text{or}\quad s=\frac{1}{2}
@answer 41
@feedback 42 | Taking only the positive square root loses the negative-coordinate branch.
@feedback 43 | Dividing each right side by 2 gives half-values, not +/-1.
@after s=-\frac{1}{2},\quad\text{or}\quad s=\frac{1}{2},\quad D:\theta\in\mathbb{R},\quad 0\le\theta<2\pi
@wrong Check the requested form, every branch and the original domain before proceeding.
@why The two linear factors give 2s=1 or 2s=-1, so s=1/2 or -1/2. Both signs are required and both values lie within the coordinate range.
@step 50 | Which set contains every permitted angle in [0,2pi), with no duplicates?
@choice 52 | S=\left\{\frac{5\pi}{6},\frac{7\pi}{6},\frac{11\pi}{6}\right\}
@choice 51 | S=\left\{\frac{\pi}{6},\frac{5\pi}{6},\frac{7\pi}{6},\frac{11\pi}{6}\right\}
@choice 53 | S=\left\{\frac{\pi}{6},\frac{5\pi}{6},\frac{7\pi}{6},\frac{11\pi}{6},0\right\}
@answer 51
@feedback 52 | This omits pi/6, where s=1/2 and s squared=1/4. It satisfies the original equation and belongs to the positive half-coordinate branch.
@feedback 53 | The extra theta=0 has s=0, so the original left side is 0 squared=0, not 1/4. Exclude that non-solution.
@after S=\left\{\frac{\pi}{6},\frac{5\pi}{6},\frac{7\pi}{6},\frac{11\pi}{6}\right\},\quad D:\theta\in\mathbb{R},\quad 0\le\theta<2\pi
@wrong Check the requested form, every branch and the original domain before proceeding.
@why Sine +1/2 occurs at pi/6 and 5pi/6; sine -1/2 occurs at 7pi/6 and 11pi/6. Each coordinate line cuts the unit circle twice. The four representatives lie in [0,2pi), and neither endpoint solves the equation.
@step 60 | Which solution count N and original-equation failure count R certify the complete set?
@choice 61 | N=4,\quad R=0
@choice 62 | N=5,\quad R=0
@choice 63 | N=4,\quad R=1
@answer 61
@feedback 62 | Each of the two half-coordinate branches contributes two points. The total is four, not five.
@feedback 63 | Each retained angle satisfies the original equality exactly; no retained angle fails. The failure count R is zero, not one.
@after S=\left\{\frac{\pi}{6},\frac{5\pi}{6},\frac{7\pi}{6},\frac{11\pi}{6}\right\},\quad N=4,\quad R=0,\quad D:\theta\in\mathbb{R},\quad 0\le\theta<2\pi
@wrong Check the requested form, every branch and the original domain before proceeding.
@why Each proposed angle has sine equal to +1/2 or -1/2. Squaring either gives the original right side 1/4. Difference-of-squares factorization proves there are exactly these two coordinate branches, each with two distinct points; hence four solutions and no original failures.
@end

@question prod04_trig_identity_equations_q04 | A squared cosine value
@template choices.v1
@version 1
@goal Find every solution of the original equation in [0,2pi). Retain its original domain, justify every branch and check the complete set.
@given {c}^{2}=\frac{1}{4},\quad 0\le\theta<2\pi
@domain Theta is real and measured in radians. The interval includes 0 and excludes 2pi. Write s=sin(theta), c=cos(theta); D is the original domain and S the complete angle set. N counts retained solutions and R counts those failing the original equation.
@read prod04_trig_identity_equations_r
@step 10 | Within the stated interval, which additional condition gives the full original domain?
@choice 11 | \theta\in\mathbb{R}
@choice 12 | s\ne0
@choice 13 | c\ne0
@answer 11
@feedback 12 | There is no division by sine in the original equation. Imposing s nonzero would exclude inputs where the original is defined.
@feedback 13 | There is no division by cosine in the original equation. Cosine-zero inputs remain part of the original domain.
@after D:\theta\in\mathbb{R},\quad 0\le\theta<2\pi
@wrong Check the requested form, every branch and the original domain before proceeding.
@why The original equation contains only sine and cosine, both defined for every real angle. There is no variable denominator, so the interval is the only restriction.
@step 20 | Clear the constant denominator and put zero on the right.
@choice 21 | \left(4\,{c}^{2}-1\right)=0
@choice 22 | \left(4\,{c}^{2}-4\right)=0
@choice 23 | \left(4\,{c}^{2}+1\right)=0
@answer 21
@feedback 22 | The right side becomes 4 times 1/4=1, not 4.
@feedback 23 | Moving the right-side 1 to the left subtracts it; it does not add 1.
@after \left(4\,{c}^{2}-1\right)=0,\quad D:\theta\in\mathbb{R},\quad 0\le\theta<2\pi
@wrong Check the requested form, every branch and the original domain before proceeding.
@why Multiply both sides of c squared=1/4 by nonzero 4 to obtain 4c squared=1. Subtract 1 to get 4c squared-1=0. This is reversible and does not restrict the domain.
@step 30 | Factor the zero-side polynomial as a product of two linear coordinate factors.
@choice 32 | \left(2\,c-1\right)\,\left(2\,c-1\right)=0
@choice 33 | \left(2\,c+1\right)\,\left(2\,c+1\right)=0
@choice 31 | \left(2\,c-1\right)\,\left(2\,c+1\right)=0
@answer 31
@feedback 32 | Squaring 2c-1 introduces a -4c term absent from the original.
@feedback 33 | Squaring 2c+1 introduces a +4c term absent from the original.
@after \left(2\,c-1\right)\,\left(2\,c+1\right)=0,\quad D:\theta\in\mathbb{R},\quad 0\le\theta<2\pi
@wrong Check the requested form, every branch and the original domain before proceeding.
@why Use difference of squares: 4c squared-1=(2c-1)(2c+1). The opposite middle terms cancel.
@step 40 | Use zero product to list every algebraic coordinate branch before range filtering.
@choice 42 | c=\frac{1}{2}
@choice 41 | c=-\frac{1}{2},\quad\text{or}\quad c=\frac{1}{2}
@choice 43 | c=-1,\quad\text{or}\quad c=1
@answer 41
@feedback 42 | Taking only the positive square root loses the negative-coordinate branch.
@feedback 43 | Dividing each right side by 2 gives half-values, not +/-1.
@after c=-\frac{1}{2},\quad\text{or}\quad c=\frac{1}{2},\quad D:\theta\in\mathbb{R},\quad 0\le\theta<2\pi
@wrong Check the requested form, every branch and the original domain before proceeding.
@why The two linear factors give 2c=1 or 2c=-1, so c=1/2 or -1/2. Both signs are required and both values lie within the coordinate range.
@step 50 | Which set contains every permitted angle in [0,2pi), with no duplicates?
@choice 51 | S=\left\{\frac{\pi}{3},\frac{2\pi}{3},\frac{4\pi}{3},\frac{5\pi}{3}\right\}
@choice 52 | S=\left\{\frac{2\pi}{3},\frac{4\pi}{3},\frac{5\pi}{3}\right\}
@choice 53 | S=\left\{\frac{\pi}{3},\frac{2\pi}{3},\frac{4\pi}{3},\frac{5\pi}{3},0\right\}
@answer 51
@feedback 52 | This omits pi/3, where c=1/2 and c squared=1/4. It is a genuine original-equation solution from the positive cosine branch.
@feedback 53 | The extra theta=0 has c=1, so the original left side is 1 squared=1, not 1/4. It is not a solution.
@after S=\left\{\frac{\pi}{3},\frac{2\pi}{3},\frac{4\pi}{3},\frac{5\pi}{3}\right\},\quad D:\theta\in\mathbb{R},\quad 0\le\theta<2\pi
@wrong Check the requested form, every branch and the original domain before proceeding.
@why Cosine +1/2 occurs at pi/3 and 5pi/3; cosine -1/2 occurs at 2pi/3 and 4pi/3. Horizontal-axis reflection supplies both points per coordinate. All four lie in [0,2pi), and no other branch exists.
@step 60 | Which solution count N and original-equation failure count R certify the complete set?
@choice 62 | N=5,\quad R=0
@choice 63 | N=4,\quad R=1
@choice 61 | N=4,\quad R=0
@answer 61
@feedback 62 | Each of the two half-coordinate branches contributes two points. The total is four, not five.
@feedback 63 | Each retained angle satisfies the original equality exactly; no retained angle fails. The failure count R is zero, not one.
@after S=\left\{\frac{\pi}{3},\frac{2\pi}{3},\frac{4\pi}{3},\frac{5\pi}{3}\right\},\quad N=4,\quad R=0,\quad D:\theta\in\mathbb{R},\quad 0\le\theta<2\pi
@wrong Check the requested form, every branch and the original domain before proceeding.
@why Each proposed angle has cosine equal to +1/2 or -1/2. Squaring either gives the original right side 1/4. Difference-of-squares factorization proves there are exactly these two coordinate branches, each with two distinct points; hence four solutions and no original failures.
@end

@question prod04_trig_identity_equations_q05 | Two coordinate terms
@template choices.v1
@version 1
@goal Find every solution of the original equation in [0,2pi). Retain its original domain, justify every branch and check the complete set.
@given \left(2\,{s}^{2}-c\right)=1,\quad 0\le\theta<2\pi
@domain Theta is real and measured in radians. The interval includes 0 and excludes 2pi. Write s=sin(theta), c=cos(theta); D is the original domain and S the complete angle set. N counts retained solutions and R counts those failing the original equation.
@read prod04_trig_identity_equations_r
@step 10 | Within the stated interval, which additional condition gives the full original domain?
@choice 12 | s\ne0
@choice 11 | \theta\in\mathbb{R}
@choice 13 | c\ne0
@answer 11
@feedback 12 | No sine denominator occurs, so s=0 is a permitted input to the original equation.
@feedback 13 | No cosine denominator occurs, so c=0 is a permitted input to the original equation.
@after D:\theta\in\mathbb{R},\quad 0\le\theta<2\pi
@wrong Check the requested form, every branch and the original domain before proceeding.
@why Only sine and cosine occur in the original equation, and both are defined for every real angle. The stated interval is the sole domain restriction.
@step 20 | Use the Pythagorean identity to write the whole equation using c only.
@choice 22 | \left(2\,\left(1+{c}^{2}\right)-c\right)=1
@choice 21 | \left(2\,\left(1-{c}^{2}\right)-c\right)=1
@choice 23 | \left(\left(1-{c}^{2}\right)-c\right)=1
@answer 21
@feedback 22 | The complementary square is 1-c squared, not 1+c squared.
@feedback 23 | The original multiplier 2 applies to the whole sine-square term and must remain.
@after \left(2\,\left(1-{c}^{2}\right)-c\right)=1,\quad D:\theta\in\mathbb{R},\quad 0\le\theta<2\pi
@wrong Check the requested form, every branch and the original domain before proceeding.
@why Since s squared+c squared=1, replace s squared by 1-c squared. The coefficient 2 multiplies the entire replacement: 2(1-c squared)-c=1.
@step 30 | Expand, move all terms to one side and make the c-squared coefficient positive.
@choice 31 | \left(\left(2\,{c}^{2}+c\right)-1\right)=0
@choice 32 | \left(\left(2\,{c}^{2}-c\right)-1\right)=0
@choice 33 | \left(\left(2\,{c}^{2}+c\right)+1\right)=0
@answer 31
@feedback 32 | Multiplication by -1 changes -c to +c, not -c.
@feedback 33 | Subtracting the original right-side 1 from the expanded constant 2 gives 2-1=1. Negating the entire equation then makes this constant -1, not +1.
@after \left(\left(2\,{c}^{2}+c\right)-1\right)=0,\quad D:\theta\in\mathbb{R},\quad 0\le\theta<2\pi
@wrong Check the requested form, every branch and the original domain before proceeding.
@why Expand to 2-2c squared-c=1. Subtract 1: 1-2c squared-c=0. Multiply by -1 to get 2c squared+c-1=0; all moves are reversible.
@step 40 | Factor the zero-side polynomial into two linear coordinate factors.
@choice 42 | \left(2\,c+1\right)\,\left(c-1\right)=0
@choice 43 | \left(2\,c-1\right)\,\left(c-1\right)=0
@choice 41 | \left(2\,c-1\right)\,\left(c+1\right)=0
@answer 41
@feedback 42 | This product expands to 2c squared-c-1, with the wrong linear sign.
@feedback 43 | Using c-1 instead of c+1 gives 2c squared-3c+1, not the required polynomial.
@after \left(2\,c-1\right)\,\left(c+1\right)=0,\quad D:\theta\in\mathbb{R},\quad 0\le\theta<2\pi
@wrong Check the requested form, every branch and the original domain before proceeding.
@why Expand (2c-1)(c+1): 2c squared+2c-c-1=2c squared+c-1. Thus the product represents the whole zero-side polynomial.
@step 50 | List every algebraic coordinate branch before applying the range restriction.
@choice 52 | c=-\frac{1}{2},\quad\text{or}\quad c=1
@choice 51 | c=\frac{1}{2},\quad\text{or}\quad c=-1
@choice 53 | c=\frac{1}{2}
@answer 51
@feedback 52 | The signs are reversed: move -1 to the right in the first factor and +1 to the right in the second.
@feedback 53 | The extreme cosine value -1 is allowed and supplies a distinct solution.
@after c=\frac{1}{2},\quad\text{or}\quad c=-1,\quad D:\theta\in\mathbb{R},\quad 0\le\theta<2\pi
@wrong Check the requested form, every branch and the original domain before proceeding.
@why From 2c-1=0, c=1/2. From c+1=0, c=-1. Both are within [-1,1]; the extreme -1 is a legitimate branch.
@step 60 | Which set contains every permitted angle in [0,2pi), without duplicates?
@choice 61 | S=\left\{\frac{\pi}{3},\pi,\frac{5\pi}{3}\right\}
@choice 62 | S=\left\{\pi,\frac{5\pi}{3}\right\}
@choice 63 | S=\left\{\frac{\pi}{3},\pi,\frac{5\pi}{3},0\right\}
@answer 61
@feedback 62 | This omits pi/3, where c=1/2 and s squared=3/4. The original gives 2(3/4)-1/2=1, so this angle must remain.
@feedback 63 | The extra theta=0 has s=0 and c=1. The original left side is 2(0 squared)-1=-1, not the required 1.
@after S=\left\{\frac{\pi}{3},\pi,\frac{5\pi}{3}\right\},\quad D:\theta\in\mathbb{R},\quad 0\le\theta<2\pi
@wrong Check the requested form, every branch and the original domain before proceeding.
@why Cosine 1/2 occurs at pi/3 and 5pi/3. Cosine -1 occurs only at pi. The two-point branch and the extreme one-point branch give three distinct interval solutions; 0 does not have either required cosine value.
@step 70 | Which solution count N and original-equation failure count R certify the complete result?
@choice 72 | N=4,\quad R=0
@choice 73 | N=3,\quad R=1
@choice 71 | N=3,\quad R=0
@answer 71
@feedback 72 | The extreme branch contributes one angle, not two; two plus one gives three.
@feedback 73 | All retained angles satisfy the original equation exactly, so R=0. The displayed failure count 1 is incorrect.
@after S=\left\{\frac{\pi}{3},\pi,\frac{5\pi}{3}\right\},\quad N=3,\quad R=0,\quad D:\theta\in\mathbb{R},\quad 0\le\theta<2\pi
@wrong Check the requested form, every branch and the original domain before proceeding.
@why For c=1/2, the circle relation gives s squared=3/4, and the original left side is 2(3/4)-1/2=1. For c=-1, s squared=0 and the original gives 0-(-1)=1. The reversible identity conversion and factorization prove these two coordinate branches complete, with three angles total.
@end

@question prod04_trig_identity_equations_q06 | A polynomial in a reciprocal
@template choices.v1
@version 1
@goal Find every solution of the original equation in [0,2pi). Retain its original domain, justify every branch and check the complete set.
@given \left(\left(3\,\sec^{2}\theta-5\,\sec\theta\right)-2\right)=0,\quad 0\le\theta<2\pi
@domain Theta is real and measured in radians. The interval includes 0 and excludes 2pi. Write s=sin(theta), c=cos(theta); D is the original domain and S the complete angle set. N counts retained solutions and R counts those failing the original equation.
@read prod04_trig_identity_equations_r
@step 10 | Within the stated interval, which additional condition gives the full original domain?
@choice 12 | \theta\in\mathbb{R}
@choice 13 | s\ne0
@choice 11 | c\ne0
@answer 11
@feedback 12 | An unrestricted domain would admit the two cosine zeros, where secant is undefined.
@feedback 13 | This excludes sine zeros rather than the cosine zeros required by secant.
@after D:c\ne0,\quad 0\le\theta<2\pi
@wrong Check the requested form, every branch and the original domain before proceeding.
@why Secant is 1/c, so every original secant term requires c nonzero. Exclude pi/2 and 3pi/2; do not add a sine restriction.
@step 20 | Rewrite the original reciprocal functions entirely with cosine denominators.
@choice 22 | \left(\left(\frac{3}{{s}^{2}}-\frac{5}{c}\right)-2\right)=0
@choice 23 | \left(\left(\frac{3}{{c}^{2}}-\frac{5}{c}\right)+2\right)=0
@choice 21 | \left(\left(\frac{3}{{c}^{2}}-\frac{5}{c}\right)-2\right)=0
@answer 21
@feedback 22 | Secant squared uses c squared, not s squared.
@feedback 23 | The original constant is -2; rewriting secant does not change that sign.
@after \left(\left(\frac{3}{{c}^{2}}-\frac{5}{c}\right)-2\right)=0,\quad D:c\ne0,\quad 0\le\theta<2\pi
@wrong Check the requested form, every branch and the original domain before proceeding.
@why Secant is 1/c and secant squared is 1/c squared. Thus the original becomes 3/c squared-5/c-2=0, on the retained domain c nonzero.
@step 30 | Multiply by nonzero c squared, then negate, to obtain an expanded polynomial equal to zero.
@choice 32 | \left(\left(2\,{c}^{2}+5\,c\right)+3\right)=0
@choice 31 | \left(\left(2\,{c}^{2}+5\,c\right)-3\right)=0
@choice 33 | \left(\left(2\,{c}^{2}-5\,c\right)-3\right)=0
@answer 31
@feedback 32 | The constant 3 becomes -3 when the whole equation is negated.
@feedback 33 | The linear term -5c becomes +5c under negation.
@after \left(\left(2\,{c}^{2}+5\,c\right)-3\right)=0,\quad D:c\ne0,\quad 0\le\theta<2\pi
@wrong Check the requested form, every branch and the original domain before proceeding.
@why Multiplication by c squared gives 3-5c-2c squared=0. Negating every term yields 2c squared+5c-3=0. Since c is nonzero on D, clearing the denominator is reversible.
@step 40 | Factor the zero-side polynomial into two linear coordinate factors.
@choice 41 | \left(2\,c-1\right)\,\left(c+3\right)=0
@choice 42 | \left(2\,c-1\right)\,\left(c-3\right)=0
@choice 43 | \left(2\,c+1\right)\,\left(c+3\right)=0
@answer 41
@feedback 42 | Replacing c+3 with c-3 gives linear coefficient -7 and constant +3.
@feedback 43 | Replacing 2c-1 with 2c+1 gives linear coefficient 7 and constant +3.
@after \left(2\,c-1\right)\,\left(c+3\right)=0,\quad D:c\ne0,\quad 0\le\theta<2\pi
@wrong Check the requested form, every branch and the original domain before proceeding.
@why The product (2c-1)(c+3) expands to 2c squared+6c-c-3=2c squared+5c-3. The middle coefficients combine to 5.
@step 50 | List every algebraic coordinate branch before applying the range restriction.
@choice 52 | c=\frac{1}{2},\quad\text{or}\quad c=3
@choice 53 | c=\frac{1}{2}
@choice 51 | c=\frac{1}{2},\quad\text{or}\quad c=-3
@answer 51
@feedback 52 | The root of c+3=0 is -3, not +3.
@feedback 53 | This is the eventual allowed branch, but it is not the complete list of algebraic roots before range filtering.
@after c=\frac{1}{2},\quad\text{or}\quad c=-3,\quad D:c\ne0,\quad 0\le\theta<2\pi
@wrong Check the requested form, every branch and the original domain before proceeding.
@why Zero product gives 2c-1=0 or c+3=0. Their algebraic roots are c=1/2 and c=-3. List both now; the next decision checks whether they are possible cosine values.
@step 60 | Which coordinate branches remain after enforcing the cosine range [-1,1]?
@choice 62 | c=\frac{1}{2},\quad\text{or}\quad c=-3
@choice 61 | c=\frac{1}{2}
@choice 63 | \varnothing
@answer 61
@feedback 62 | Cosine cannot equal -3; keeping that algebraic root confuses polynomial roots with permitted trigonometric values.
@feedback 63 | Cosine 1/2 is possible and nonzero, so declaring all branches impossible loses two genuine solutions.
@after c=\frac{1}{2},\quad D:c\ne0,\quad 0\le\theta<2\pi
@wrong Check the requested form, every branch and the original domain before proceeding.
@why The value 1/2 is within [-1,1], while -3 is below the minimum possible cosine -1. Reject the latter branch; it cannot supply an angle. The remaining c=1/2 is also nonzero, so it respects D.
@step 70 | Which set contains every permitted angle in [0,2pi), without duplicates?
@choice 71 | S=\left\{\frac{\pi}{3},\frac{5\pi}{3}\right\}
@choice 72 | S=\left\{\frac{5\pi}{3}\right\}
@choice 73 | S=\left\{\frac{\pi}{3},\frac{5\pi}{3},0\right\}
@answer 71
@feedback 72 | This omits pi/3, where c=1/2 and secant is 2. The original gives 3(2 squared)-5(2)-2=12-10-2=0, so it is a required solution.
@feedback 73 | At the extra theta=0, cosine is 1 and secant is defined with value 1. The original gives 3(1 squared)-5(1)-2=-4, not 0; this is an equation failure, not a domain exclusion.
@after S=\left\{\frac{\pi}{3},\frac{5\pi}{3}\right\},\quad D:c\ne0,\quad 0\le\theta<2\pi
@wrong Check the requested form, every branch and the original domain before proceeding.
@why The only allowed coordinate is c=1/2. Its horizontal-axis reflected angles are pi/3 and 5pi/3. Both have nonzero cosine and belong to [0,2pi); the coordinate-line intersection count proves there are exactly two.
@step 80 | Which solution count N and original-equation failure count R certify the complete result?
@choice 82 | N=3,\quad R=0
@choice 83 | N=2,\quad R=1
@choice 81 | N=2,\quad R=0
@answer 81
@feedback 82 | The rejected c=-3 branch contributes zero angles; the c=1/2 branch contributes exactly two, not three.
@feedback 83 | All retained angles satisfy the original equation exactly, so R=0. The displayed failure count 1 is incorrect.
@after S=\left\{\frac{\pi}{3},\frac{5\pi}{3}\right\},\quad N=2,\quad R=0,\quad D:c\ne0,\quad 0\le\theta<2\pi
@wrong Check the requested form, every branch and the original domain before proceeding.
@why Both retained angles have c=1/2, hence secant 2. Substitution into the original gives 3(2 squared)-5(2)-2=12-10-2=0. The other algebraic cosine value -3 is impossible, so these two angles are the complete original solution set.
@end

@question prod04_trig_identity_equations_q07 | A squared reciprocal value
@template choices.v1
@version 1
@goal Find every solution of the original equation in [0,2pi). Retain its original domain, justify every branch and check the complete set.
@given \sec^{2}\theta=4,\quad 0\le\theta<2\pi
@domain Theta is real and measured in radians. The interval includes 0 and excludes 2pi. Write s=sin(theta), c=cos(theta); D is the original domain and S the complete angle set. N counts retained solutions and R counts those failing the original equation.
@read prod04_trig_identity_equations_r
@step 10 | Within the stated interval, which additional condition gives the full original domain?
@choice 11 | c\ne0
@choice 12 | \theta\in\mathbb{R}
@choice 13 | s\ne0
@answer 11
@feedback 12 | An unrestricted domain would admit the two cosine zeros, where secant is undefined.
@feedback 13 | This excludes sine zeros rather than the cosine zeros required by secant.
@after D:c\ne0,\quad 0\le\theta<2\pi
@wrong Check the requested form, every branch and the original domain before proceeding.
@why Secant is 1/c, so every original secant term requires c nonzero. Exclude pi/2 and 3pi/2; do not add a sine restriction.
@step 20 | Use the reciprocal definition to rewrite the equation in c.
@choice 22 | \frac{1}{c}=4
@choice 21 | \frac{1}{{c}^{2}}=4
@choice 23 | \frac{1}{{s}^{2}}=4
@answer 21
@feedback 22 | This is secant, not secant squared; the denominator square is missing.
@feedback 23 | Cosecant uses sine, but the original function is secant with a cosine denominator.
@after \frac{1}{{c}^{2}}=4,\quad D:c\ne0,\quad 0\le\theta<2\pi
@wrong Check the requested form, every branch and the original domain before proceeding.
@why Secant squared is the square of 1/c, namely 1/c squared. The square belongs to the whole reciprocal, and c remains nonzero.
@step 30 | Clear the nonzero denominator and place the polynomial with positive leading coefficient on the left.
@choice 31 | \left(4\,{c}^{2}-1\right)=0
@choice 32 | \left({c}^{2}-4\right)=0
@choice 33 | \left(4\,{c}^{2}+1\right)=0
@answer 31
@feedback 32 | The coefficient 4 multiplies c squared after clearing the denominator; the equation is not c squared=4.
@feedback 33 | Moving 1 to the left of 4c squared subtracts it, giving 4c squared-1.
@after \left(4\,{c}^{2}-1\right)=0,\quad D:c\ne0,\quad 0\le\theta<2\pi
@wrong Check the requested form, every branch and the original domain before proceeding.
@why From 1/c squared=4, multiply by nonzero c squared to get 1=4c squared. Rearrange to 4c squared-1=0, retaining the original domain.
@step 40 | Factor the zero-side polynomial into two linear coordinate factors.
@choice 42 | \left(2\,c-1\right)\,\left(2\,c-1\right)=0
@choice 43 | \left(2\,c+1\right)\,\left(2\,c+1\right)=0
@choice 41 | \left(2\,c-1\right)\,\left(2\,c+1\right)=0
@answer 41
@feedback 42 | A repeated negative factor creates a linear -4c term.
@feedback 43 | A repeated positive factor creates a linear +4c term.
@after \left(2\,c-1\right)\,\left(2\,c+1\right)=0,\quad D:c\ne0,\quad 0\le\theta<2\pi
@wrong Check the requested form, every branch and the original domain before proceeding.
@why The difference of squares factors as (2c-1)(2c+1). Expanding gives 4c squared+2c-2c-1=4c squared-1.
@step 50 | List every algebraic coordinate branch before applying the range restriction.
@choice 52 | c=\frac{1}{2}
@choice 51 | c=-\frac{1}{2},\quad\text{or}\quad c=\frac{1}{2}
@choice 53 | c=-2,\quad\text{or}\quad c=2
@answer 51
@feedback 52 | The negative cosine branch also gives secant squared 4 and must remain.
@feedback 53 | Solving 2c=+/-1 gives c=+/-1/2, not +/-2.
@after c=-\frac{1}{2},\quad\text{or}\quad c=\frac{1}{2},\quad D:c\ne0,\quad 0\le\theta<2\pi
@wrong Check the requested form, every branch and the original domain before proceeding.
@why Each factor can vanish: 2c-1=0 gives c=1/2, while 2c+1=0 gives c=-1/2. Both are allowed nonzero cosine values.
@step 60 | Which set contains every permitted angle in [0,2pi), without duplicates?
@choice 61 | S=\left\{\frac{\pi}{3},\frac{2\pi}{3},\frac{4\pi}{3},\frac{5\pi}{3}\right\}
@choice 62 | S=\left\{\frac{2\pi}{3},\frac{4\pi}{3},\frac{5\pi}{3}\right\}
@choice 63 | S=\left\{\frac{\pi}{3},\frac{2\pi}{3},\frac{4\pi}{3},\frac{5\pi}{3},0\right\}
@answer 61
@feedback 62 | This omits pi/3, where c=1/2 and secant is 2. Its square is 4, exactly the original right side, so retain the angle.
@feedback 63 | At the extra theta=0, c=1 and secant is defined with value 1. Its square is 1, not 4, so the angle fails the original equation.
@after S=\left\{\frac{\pi}{3},\frac{2\pi}{3},\frac{4\pi}{3},\frac{5\pi}{3}\right\},\quad D:c\ne0,\quad 0\le\theta<2\pi
@wrong Check the requested form, every branch and the original domain before proceeding.
@why Cosine +1/2 gives pi/3 and 5pi/3; cosine -1/2 gives 2pi/3 and 4pi/3. None is a cosine zero. Two points per coordinate give four distinct interval angles.
@step 70 | Which solution count N and original-equation failure count R certify the complete result?
@choice 72 | N=5,\quad R=0
@choice 73 | N=4,\quad R=1
@choice 71 | N=4,\quad R=0
@answer 71
@feedback 72 | Two non-extreme coordinate branches each contribute two angles, totaling four rather than five.
@feedback 73 | All retained angles satisfy the original equation exactly, so R=0. The displayed failure count 1 is incorrect.
@after S=\left\{\frac{\pi}{3},\frac{2\pi}{3},\frac{4\pi}{3},\frac{5\pi}{3}\right\},\quad N=4,\quad R=0,\quad D:c\ne0,\quad 0\le\theta<2\pi
@wrong Check the requested form, every branch and the original domain before proceeding.
@why The four angles have cosine +/-1/2, so secant is +/-2 and its square is 4 in the original equation. The factored quadratic has only these two nonzero coordinate roots, each with two circle intersections. Hence N=4 and every original residual is zero.
@end

@question prod04_trig_identity_equations_q08 | Another squared reciprocal value
@template choices.v1
@version 1
@goal Find every solution of the original equation in [0,2pi). Retain its original domain, justify every branch and check the complete set.
@given \csc^{2}\theta=\frac{1}{4},\quad 0\le\theta<2\pi
@domain Theta is real and measured in radians. The interval includes 0 and excludes 2pi. Write s=sin(theta), c=cos(theta); D is the original domain and S the complete angle set. N counts retained solutions and R counts those failing the original equation.
@read prod04_trig_identity_equations_r
@step 10 | Within the stated interval, which additional condition gives the full original domain?
@choice 12 | \theta\in\mathbb{R}
@choice 11 | s\ne0
@choice 13 | c\ne0
@answer 11
@feedback 12 | An unrestricted domain would admit sine zeros, where cosecant is undefined.
@feedback 13 | This excludes cosine zeros rather than the sine zeros required by cosecant.
@after D:s\ne0,\quad 0\le\theta<2\pi
@wrong Check the requested form, every branch and the original domain before proceeding.
@why Cosecant is 1/s, so the original requires s nonzero. Exclude 0 and pi; 2pi is already excluded by the half-open interval.
@step 20 | Rewrite the squared reciprocal as a coordinate fraction.
@choice 22 | \frac{1}{s}=\frac{1}{4}
@choice 23 | \frac{1}{{c}^{2}}=\frac{1}{4}
@choice 21 | \frac{1}{{s}^{2}}=\frac{1}{4}
@answer 21
@feedback 22 | The original function is squared, so its denominator must be s squared rather than s.
@feedback 23 | A cosine denominator would describe secant squared, not cosecant squared.
@after \frac{1}{{s}^{2}}=\frac{1}{4},\quad D:s\ne0,\quad 0\le\theta<2\pi
@wrong Check the requested form, every branch and the original domain before proceeding.
@why Cosecant is 1/s, so its square is 1/s squared. The right side remains 1/4, and original sine zeros remain excluded.
@step 30 | Clear the constant and nonzero sine denominators to put a polynomial with leading coefficient 1 on the left.
@choice 32 | \left(4\,{s}^{2}-1\right)=0
@choice 31 | \left({s}^{2}-4\right)=0
@choice 33 | \left({s}^{2}+4\right)=0
@answer 31
@feedback 32 | The reciprocal equation gives s squared=4, not 1/4. Inverting a reciprocal changes which side carries the factor 4.
@feedback 33 | Moving 4 from the right to the left subtracts it, so the constant is -4.
@after \left({s}^{2}-4\right)=0,\quad D:s\ne0,\quad 0\le\theta<2\pi
@wrong Check the requested form, every branch and the original domain before proceeding.
@why Multiply 1/s squared=1/4 by 4s squared, which is nonzero on D: 4=s squared. Rearrange to s squared-4=0. This step is reversible on the original domain.
@step 40 | Factor the zero-side polynomial into two linear coordinate factors.
@choice 41 | \left(s-2\right)\,\left(s+2\right)=0
@choice 42 | \left(s-2\right)\,\left(s-2\right)=0
@choice 43 | \left(s+2\right)\,\left(s+2\right)=0
@answer 41
@feedback 42 | Squaring s-2 introduces -4s and a positive constant 4.
@feedback 43 | Squaring s+2 introduces +4s and a positive constant 4.
@after \left(s-2\right)\,\left(s+2\right)=0,\quad D:s\ne0,\quad 0\le\theta<2\pi
@wrong Check the requested form, every branch and the original domain before proceeding.
@why Difference of squares gives s squared-4=(s-2)(s+2). Expanding cancels the opposite linear terms.
@step 50 | List every algebraic coordinate branch before applying the range restriction.
@choice 52 | s=2
@choice 53 | s=-\frac{1}{2},\quad\text{or}\quad s=\frac{1}{2}
@choice 51 | s=-2,\quad\text{or}\quad s=2
@answer 51
@feedback 52 | Both linear factors have roots; list -2 as well before applying the range test.
@feedback 53 | The roots of s squared-4=0 are +/-2, not their reciprocals.
@after s=-2,\quad\text{or}\quad s=2,\quad D:s\ne0,\quad 0\le\theta<2\pi
@wrong Check the requested form, every branch and the original domain before proceeding.
@why Zero product gives s-2=0 or s+2=0, so the algebraic candidates are +2 and -2. Their existence as algebraic roots does not make them possible sine coordinates.
@step 60 | Which algebraic branches remain inside the sine range [-1,1]?
@choice 62 | s=-2,\quad\text{or}\quad s=2
@choice 61 | \varnothing
@choice 63 | s=2
@answer 61
@feedback 62 | Both values have magnitude 2, exceeding the maximum possible sine magnitude 1.
@feedback 63 | The positive branch is just as impossible as the negative one; sine cannot be 2.
@after \varnothing,\quad D:s\ne0,\quad 0\le\theta<2\pi
@wrong Check the requested form, every branch and the original domain before proceeding.
@why Neither -2 nor 2 lies in [-1,1]. Therefore no sine branch is admissible and the solution set is empty. This is a range contradiction for every angle, not a failed search through familiar angles.
@step 70 | Which solution count N and original-equation failure count R certify the complete result?
@choice 71 | N=0,\quad R=0
@choice 72 | N=1,\quad R=0
@choice 73 | N=0,\quad R=1
@answer 71
@feedback 72 | An impossible pair of sine values contributes no angle. One familiar angle cannot overcome the range contradiction.
@feedback 73 | There are no retained angles, so none can fail substitution: R=0. The no-solution proof comes from impossible coordinate branches, not from a claimed failed retained angle.
@after S=\varnothing,\quad N=0,\quad R=0,\quad D:s\ne0,\quad 0\le\theta<2\pi
@wrong Check the requested form, every branch and the original domain before proceeding.
@why The original equation implies s squared=4 after multiplying by nonzero 4s squared. But every real angle has 0<=s squared<=1. This contradiction excludes every angle in the original domain. Thus S is empty, N=0, and there are no retained angles that could fail substitution, so R=0.
@end

@question prod04_trig_identity_equations_q09 | A product equation
@template choices.v1
@version 1
@goal Select a useful algebraic or identity strategy and find every original-equation solution in [0,2pi). Retain D and zero-factor branches; verify the complete set.
@given 2\,s\,c=c,\quad 0\le\theta<2\pi
@domain Theta is real and measured in radians. The interval includes 0 and excludes 2pi. Write s=sin(theta), c=cos(theta); D is the original domain and S the complete angle set. N counts retained solutions and R counts those failing the original equation.
@read prod04_trig_identity_equations_r
@step 10 | Which additional restriction is required by the original expression, within the stated interval?
@choice 12 | s\ne0
@choice 13 | c\ne0
@choice 11 | \theta\in\mathbb{R}
@answer 11
@feedback 12 | Sine zero is permitted in the original equation; a later desire to divide does not justify excluding it.
@feedback 13 | Cosine zero is permitted in the original equation; a later algebraic step must not erase this possible branch.
@after D:\theta\in\mathbb{R},\quad 0\le\theta<2\pi
@wrong Check the requested form, every branch and the original domain before proceeding.
@why The original uses only products of sine and cosine, not quotients. Both functions exist at every real angle, so no additional restriction is needed.
@step 20 | Which strategy preserves exactly the original solutions, without losing zero-factor roots or introducing new ones?
@choice 21 | \text{factor }\left(2\,s\,c-c\right)
@choice 22 | \text{divide by }c
@choice 23 | 4\,{s}^{2}\,{c}^{2}={c}^{2}
@answer 21
@feedback 22 | Dividing by c loses the original solution pi/2: there c=0 and both original sides are zero, but division by c is undefined. Factoring retains this zero branch.
@feedback 23 | Squaring both sides introduces extra roots. At 7pi/6, s=-1/2 and c=-sqrt(3)/2, so the squared equation holds, but the original sides are +sqrt(3)/2 and -sqrt(3)/2. Squaring could be used with later rejection of extra roots, but it misses this reversible-strategy goal.
@after c\,\left(2\,s-1\right)=0,\quad D:\theta\in\mathbb{R},\quad 0\le\theta<2\pi
@wrong Check the requested form, every branch and the original domain before proceeding.
@why Subtracting c and factoring are reversible without a nonzero assumption on c: 2sc-c=c(2s-1). This strategy preserves the zero-factor branch and creates no new solutions. The reached product executes the selected strategy; the next decision solves both factors.
@step 30 | Use zero product to list all coordinate branches, retaining possible zero factors.
@choice 32 | s=\frac{1}{2}
@choice 33 | c=0,\quad\text{or}\quad s=-\frac{1}{2}
@choice 31 | c=0,\quad\text{or}\quad s=\frac{1}{2}
@answer 31
@feedback 32 | This loses the c=0 branch by dividing out a factor that can be zero.
@feedback 33 | Solving 2s-1=0 gives s=1/2, not -1/2.
@after c=0,\quad\text{or}\quad s=\frac{1}{2},\quad D:\theta\in\mathbb{R},\quad 0\le\theta<2\pi
@wrong Check the requested form, every branch and the original domain before proceeding.
@why Zero product gives c=0 or 2s-1=0, so s=1/2. Do not divide by c: cosine-zero angles satisfy both original sides and must remain.
@step 40 | Which set includes every allowed angle in [0,2pi), with no repeated branch or endpoint?
@choice 42 | S=\left\{\frac{\pi}{2},\frac{5\pi}{6},\frac{3\pi}{2}\right\}
@choice 41 | S=\left\{\frac{\pi}{6},\frac{\pi}{2},\frac{5\pi}{6},\frac{3\pi}{2}\right\}
@choice 43 | S=\left\{0,\frac{\pi}{6},\frac{\pi}{2},\frac{5\pi}{6},\frac{3\pi}{2}\right\}
@answer 41
@feedback 42 | This omits pi/6 from the sine-half branch; the remaining angles are not complete.
@feedback 43 | At theta=0, s=0 and c=1, so the original would say 0=1. That extra angle is not a solution.
@after S=\left\{\frac{\pi}{6},\frac{\pi}{2},\frac{5\pi}{6},\frac{3\pi}{2}\right\},\quad D:\theta\in\mathbb{R},\quad 0\le\theta<2\pi
@wrong Check the requested form, every branch and the original domain before proceeding.
@why The c=0 branch gives pi/2 and 3pi/2. The s=1/2 branch gives pi/6 and 5pi/6. These branches do not overlap, since c=0 forces s=+/-1, not 1/2. Four distinct interval points result.
@step 50 | Which solution count N and original-equation failure count R complete the proof?
@choice 51 | N=4,\quad R=0
@choice 52 | N=5,\quad R=0
@choice 53 | N=4,\quad R=1
@answer 51
@feedback 52 | There are two cosine-zero points and two sine-half points; none overlaps, so the total is four rather than five.
@feedback 53 | Exact substitution verifies all four original equalities, so no retained angle fails: R=0.
@after S=\left\{\frac{\pi}{6},\frac{\pi}{2},\frac{5\pi}{6},\frac{3\pi}{2}\right\},\quad N=4,\quad R=0,\quad D:\theta\in\mathbb{R},\quad 0\le\theta<2\pi
@wrong Check the requested form, every branch and the original domain before proceeding.
@why For c=0, the original equality is 2s(0)=0. For s=1/2, it is 2(1/2)c=c. Thus all four angles satisfy the original exactly. The verified factorization gives only these two branches, each with two nonoverlapping circle points.
@end

@question prod04_trig_identity_equations_q10 | A signed product equation
@template choices.v1
@version 1
@goal Select a useful algebraic or identity strategy and find every original-equation solution in [0,2pi). Retain D and zero-factor branches; verify the complete set.
@given -1\,s=2\,c\,s,\quad 0\le\theta<2\pi
@domain Theta is real and measured in radians. The interval includes 0 and excludes 2pi. Write s=sin(theta), c=cos(theta); D is the original domain and S the complete angle set. N counts retained solutions and R counts those failing the original equation.
@read prod04_trig_identity_equations_r
@step 10 | Which additional restriction is required by the original expression, within the stated interval?
@choice 11 | \theta\in\mathbb{R}
@choice 12 | s\ne0
@choice 13 | c\ne0
@answer 11
@feedback 12 | Sine zero is permitted in the original equation; a later desire to divide does not justify excluding it.
@feedback 13 | Cosine zero is permitted in the original equation; a later algebraic step must not erase this possible branch.
@after D:\theta\in\mathbb{R},\quad 0\le\theta<2\pi
@wrong Check the requested form, every branch and the original domain before proceeding.
@why The original uses only products of sine and cosine, not quotients. Both functions exist at every real angle, so no additional restriction is needed.
@step 20 | Select a zero-preserving strategy: move all terms left, negate and factor the common coordinate.
@choice 22 | s\,\left(1-2\,c\right)=0
@choice 23 | c\,\left(1+2\,s\right)=0
@choice 21 | s\,\left(1+2\,c\right)=0
@answer 21
@feedback 22 | Negating -s-2cs gives s+2cs, not s-2cs.
@feedback 23 | The common factor is s. Factoring c instead gives c+2sc, which is a different polynomial.
@after s\,\left(1+2\,c\right)=0,\quad D:\theta\in\mathbb{R},\quad 0\le\theta<2\pi
@wrong Check the requested form, every branch and the original domain before proceeding.
@why Move terms to obtain -s-2cs=0, then negate: s+2cs=0. Factor s to get s(1+2c)=0. These reversible operations retain s=0; dividing by s would lose that branch.
@step 30 | Use zero product to list every coordinate branch.
@choice 32 | c=-\frac{1}{2}
@choice 31 | s=0,\quad\text{or}\quad c=-\frac{1}{2}
@choice 33 | s=0,\quad\text{or}\quad c=\frac{1}{2}
@answer 31
@feedback 32 | The original equation holds when s=0; division by s would silently discard those roots.
@feedback 33 | Moving +1 to the right gives 2c=-1, so c is negative one half.
@after s=0,\quad\text{or}\quad c=-\frac{1}{2},\quad D:\theta\in\mathbb{R},\quad 0\le\theta<2\pi
@wrong Check the requested form, every branch and the original domain before proceeding.
@why Either s=0 or 1+2c=0. The second branch gives 2c=-1, hence c=-1/2. Both branches are required by zero product.
@step 40 | Which set includes every allowed angle in [0,2pi)?
@choice 41 | S=\left\{0,\frac{2\pi}{3},\pi,\frac{4\pi}{3}\right\}
@choice 42 | S=\left\{\frac{2\pi}{3},\pi,\frac{4\pi}{3}\right\}
@choice 43 | S=\left\{0,\frac{\pi}{2},\frac{2\pi}{3},\pi,\frac{4\pi}{3}\right\}
@answer 41
@feedback 42 | The lower endpoint 0 is included and solves the original equation; it must remain.
@feedback 43 | At pi/2, s=1 and c=0, so the original would say -1=0. The extra angle is not a solution.
@after S=\left\{0,\frac{2\pi}{3},\pi,\frac{4\pi}{3}\right\},\quad D:\theta\in\mathbb{R},\quad 0\le\theta<2\pi
@wrong Check the requested form, every branch and the original domain before proceeding.
@why Sine zero gives 0 and pi; cosine -1/2 gives 2pi/3 and 4pi/3. The branches do not overlap because sine zero forces cosine +/-1. Keep zero, exclude 2pi, and retain all four distinct angles.
@step 50 | Which solution count N and original-equation failure count R complete the proof?
@choice 52 | N=5,\quad R=0
@choice 53 | N=4,\quad R=1
@choice 51 | N=4,\quad R=0
@answer 51
@feedback 52 | Counting 2pi alongside 0 would repeat the same direction at an excluded endpoint. There are four permitted angles.
@feedback 53 | All retained angles satisfy the original equality, so R is zero.
@after S=\left\{0,\frac{2\pi}{3},\pi,\frac{4\pi}{3}\right\},\quad N=4,\quad R=0,\quad D:\theta\in\mathbb{R},\quad 0\le\theta<2\pi
@wrong Check the requested form, every branch and the original domain before proceeding.
@why When s=0, both original sides are zero. When c=-1/2, the right side is 2(-1/2)s=-s, exactly the left side. Zero product proves there are no other branches, and their nonoverlapping two-point sets give four solutions.
@end

@question prod04_trig_identity_equations_q11 | A reciprocal product equation
@template choices.v1
@version 1
@goal Select a useful algebraic or identity strategy and find every original-equation solution in [0,2pi). Retain D and zero-factor branches; verify the complete set.
@given \left(\sec\theta\,s-2\,s\right)=0,\quad 0\le\theta<2\pi
@domain Theta is real and measured in radians. The interval includes 0 and excludes 2pi. Write s=sin(theta), c=cos(theta); D is the original domain and S the complete angle set. N counts retained solutions and R counts those failing the original equation.
@read prod04_trig_identity_equations_r
@step 10 | Which additional condition preserves the original reciprocal domain?
@choice 12 | \theta\in\mathbb{R}
@choice 11 | c\ne0
@choice 13 | s\ne0
@answer 11
@feedback 12 | Cosine zeros make secant undefined, even if later algebra no longer displays a denominator.
@feedback 13 | Sine zeros are allowed here. The reciprocal denominator is cosine, not sine.
@after D:c\ne0,\quad 0\le\theta<2\pi
@wrong Check the requested form, every branch and the original domain before proceeding.
@why The original secant is 1/c, so c must be nonzero. This excludes pi/2 and 3pi/2. It does not exclude s=0, where secant remains defined.
@step 20 | Which initial strategy keeps every original solution and introduces no extra roots? The last choice moves 2s right and squares both sides.
@choice 21 | \sec\theta=\frac{1}{c}
@choice 22 | \text{divide by }s
@choice 23 | \sec^{2}\theta\,{s}^{2}=4\,{s}^{2}
@answer 21
@feedback 22 | Dividing by s loses theta=0, where s=0, c=1 and the original difference is 1(0)-2(0)=0. This zero-sine root is allowed by the original cosine restriction.
@feedback 23 | Squaring the isolated equation admits 2pi/3: secant is -2 and s=sqrt(3)/2, so both squared sides are 3. Yet the original difference is -sqrt(3)-sqrt(3)=-2sqrt(3), not 0. That method would require removing added roots and misses the stated reversible-strategy goal.
@after \left(\frac{s}{c}-2\,s\right)=0,\quad D:c\ne0,\quad 0\le\theta<2\pi
@wrong Check the requested form, every branch and the original domain before proceeding.
@why The reciprocal identity secant=1/c is valid everywhere on the original domain c nonzero. Applying it gives s/c-2s=0 without losing zero-sine roots or adding solutions. The next decision executes denominator clearing with the already nonzero c; factoring then preserves every sine branch.
@step 30 | Multiply by the nonzero original denominator and expand, without dividing by sine.
@choice 32 | \left(s-2\,s\right)=0
@choice 33 | \left(s+2\,s\,c\right)=0
@choice 31 | \left(s-2\,s\,c\right)=0
@answer 31
@feedback 32 | The term -2s also receives the multiplier c, giving -2sc.
@feedback 33 | The original -2s term remains negative after multiplication by c.
@after \left(s-2\,s\,c\right)=0,\quad D:c\ne0,\quad 0\le\theta<2\pi
@wrong Check the requested form, every branch and the original domain before proceeding.
@why Multiply every term by c: (s/c)c-2sc=0, so s-2sc=0. This is reversible on D because c is nonzero. No division by s is needed.
@step 40 | Factor the polynomial without discarding any zero-coordinate branch.
@choice 42 | s\,\left(1+2\,c\right)=0
@choice 41 | s\,\left(1-2\,c\right)=0
@choice 43 | c\,\left(1-2\,s\right)=0
@answer 41
@feedback 42 | The linear cosine contribution is negative: the binomial must be 1-2c.
@feedback 43 | Factoring c would give c-2sc, not the required s-2sc.
@after s\,\left(1-2\,c\right)=0,\quad D:c\ne0,\quad 0\le\theta<2\pi
@wrong Check the requested form, every branch and the original domain before proceeding.
@why Both terms in s-2sc contain s, so factor it as s(1-2c)=0. Retain the original cosine restriction even though this line has no denominator.
@step 50 | Use zero product to keep every coordinate branch.
@choice 51 | s=0,\quad\text{or}\quad c=\frac{1}{2}
@choice 52 | c=\frac{1}{2}
@choice 53 | s=0,\quad\text{or}\quad c=-\frac{1}{2}
@answer 51
@feedback 52 | This loses the sine-zero roots even though the original secant is defined at those angles.
@feedback 53 | From 1-2c=0, 2c=1 and c=1/2; the value is positive.
@after s=0,\quad\text{or}\quad c=\frac{1}{2},\quad D:c\ne0,\quad 0\le\theta<2\pi
@wrong Check the requested form, every branch and the original domain before proceeding.
@why The zero factors are s=0 or 1-2c=0. Solving the second yields c=1/2. The sine-zero branch has cosine +/-1, so it is fully compatible with the original domain c nonzero.
@step 60 | Which set contains every angle allowed by both the factor branches and the original domain?
@choice 62 | S=\left\{\frac{\pi}{3},\pi,\frac{5\pi}{3}\right\}
@choice 63 | S=\left\{0,\frac{\pi}{3},\frac{\pi}{2},\pi,\frac{5\pi}{3}\right\}
@choice 61 | S=\left\{0,\frac{\pi}{3},\pi,\frac{5\pi}{3}\right\}
@answer 61
@feedback 62 | Zero is an included, valid sine-zero root, so omitting it makes the set incomplete.
@feedback 63 | At pi/2 cosine is zero and the original secant is undefined. Denominator clearing cannot restore that angle.
@after S=\left\{0,\frac{\pi}{3},\pi,\frac{5\pi}{3}\right\},\quad D:c\ne0,\quad 0\le\theta<2\pi
@wrong Check the requested form, every branch and the original domain before proceeding.
@why Sine zero gives 0 and pi, with cosine +/-1. Cosine 1/2 gives pi/3 and 5pi/3. None is a cosine zero, and the branches do not overlap. Keep the included endpoint 0 and exclude 2pi.
@step 70 | Which counts certify the complete original solution set?
@choice 72 | N=5,\quad R=0
@choice 71 | N=4,\quad R=0
@choice 73 | N=4,\quad R=1
@answer 71
@feedback 72 | The original domain removes cosine-zero angles; it does not create an extra fifth solution. Each surviving branch contributes two.
@feedback 73 | Every retained original substitution is exact, so the failure count is zero.
@after S=\left\{0,\frac{\pi}{3},\pi,\frac{5\pi}{3}\right\},\quad N=4,\quad R=0,\quad D:c\ne0,\quad 0\le\theta<2\pi
@wrong Check the requested form, every branch and the original domain before proceeding.
@why At s=0 the original secant product and 2s are both zero, with c=+/-1 so secant exists. At c=1/2, secant is 2 and the original difference is 2s-2s=0. These complete, disjoint branches give four allowed angles and no substitution failures.
@end

@question prod04_trig_identity_equations_q12 | An equation with cotangent
@template choices.v1
@version 1
@goal Select a useful algebraic or identity strategy and find every original-equation solution in [0,2pi). Retain D and zero-factor branches; verify the complete set.
@given 2\,c=\cot\theta,\quad 0\le\theta<2\pi
@domain Theta is real and measured in radians. The interval includes 0 and excludes 2pi. Write s=sin(theta), c=cos(theta); D is the original domain and S the complete angle set. N counts retained solutions and R counts those failing the original equation.
@read prod04_trig_identity_equations_r
@step 10 | Which additional condition is required by the original cotangent?
@choice 12 | \theta\in\mathbb{R}
@choice 13 | c\ne0
@choice 11 | s\ne0
@answer 11
@feedback 12 | Sine-zero angles make cotangent undefined and cannot be restored after denominator clearing.
@feedback 13 | The denominator of cotangent is sine, not cosine. Cosine-zero angles are allowed and must be considered.
@after D:s\ne0,\quad 0\le\theta<2\pi
@wrong Check the requested form, every branch and the original domain before proceeding.
@why Cotangent is c/s, so the original requires s nonzero. Exclude 0 and pi; the interval excludes 2pi already. Cosine may be zero while cotangent is defined.
@step 20 | Choose the quotient-definition strategy that exposes the original denominator.
@choice 22 | 2\,c=\frac{s}{c}
@choice 21 | 2\,c=\frac{c}{s}
@choice 23 | 2\,c=\frac{1}{s}
@answer 21
@feedback 22 | The ratio s/c is tangent, not cotangent.
@feedback 23 | The reciprocal 1/s is cosecant; cotangent also has the cosine numerator.
@after 2\,c=\frac{c}{s},\quad D:s\ne0,\quad 0\le\theta<2\pi
@wrong Check the requested form, every branch and the original domain before proceeding.
@why Use cotangent=c/s. The equation becomes 2c=c/s. This identifies s as the original nonzero denominator before multiplication.
@step 30 | Clear the nonzero sine denominator and put an expanded expression equal to zero.
@choice 31 | \left(2\,c\,s-c\right)=0
@choice 32 | \left(2\,c-c\right)=0
@choice 33 | \left(2\,c\,s+c\right)=0
@answer 31
@feedback 32 | The left side must also be multiplied by s, giving 2cs rather than 2c.
@feedback 33 | Moving the right-side c left subtracts it, not adds it.
@after \left(2\,c\,s-c\right)=0,\quad D:s\ne0,\quad 0\le\theta<2\pi
@wrong Check the requested form, every branch and the original domain before proceeding.
@why Multiply both sides by nonzero s: 2cs=c. Subtract c to obtain 2cs-c=0. Retain s nonzero, and do not divide by c, which may vanish.
@step 40 | Factor the expanded equation without dividing by a possibly zero coordinate.
@choice 42 | c\,\left(2\,s+1\right)=0
@choice 43 | s\,\left(2\,c-1\right)=0
@choice 41 | c\,\left(2\,s-1\right)=0
@answer 41
@feedback 42 | The second factor is 2s-1; using +1 changes the sign of the original -c term.
@feedback 43 | The subtracted term is c, so the common factor is c rather than s.
@after c\,\left(2\,s-1\right)=0,\quad D:s\ne0,\quad 0\le\theta<2\pi
@wrong Check the requested form, every branch and the original domain before proceeding.
@why Factor the common c from 2cs-c: c(2s-1)=0. This preserves the cosine-zero branch while retaining the original sine restriction.
@step 50 | Which coordinate branches follow from zero product on the original domain?
@choice 52 | s=\frac{1}{2}
@choice 51 | c=0,\quad\text{or}\quad s=\frac{1}{2}
@choice 53 | c=0,\quad\text{or}\quad s=-\frac{1}{2}
@answer 51
@feedback 52 | Cosine zero is allowed in the original cotangent domain. Dropping it loses two valid roots.
@feedback 53 | Solving 2s-1=0 gives the positive half-value, not the negative one.
@after c=0,\quad\text{or}\quad s=\frac{1}{2},\quad D:s\ne0,\quad 0\le\theta<2\pi
@wrong Check the requested form, every branch and the original domain before proceeding.
@why Zero product gives c=0 or 2s-1=0, hence s=1/2. When c=0 the circle relation gives s=+/-1, so both cosine-zero points satisfy the original sine exclusion.
@step 60 | Which set contains all distinct original-domain solutions in [0,2pi)?
@choice 61 | S=\left\{\frac{\pi}{6},\frac{\pi}{2},\frac{5\pi}{6},\frac{3\pi}{2}\right\}
@choice 62 | S=\left\{\frac{\pi}{2},\frac{5\pi}{6},\frac{3\pi}{2}\right\}
@choice 63 | S=\left\{0,\frac{\pi}{6},\frac{\pi}{2},\frac{5\pi}{6},\frac{3\pi}{2}\right\}
@answer 61
@feedback 62 | This omits pi/6 from the admissible sine-half branch.
@feedback 63 | The extra zero angle has sine zero, so the original cotangent is undefined there.
@after S=\left\{\frac{\pi}{6},\frac{\pi}{2},\frac{5\pi}{6},\frac{3\pi}{2}\right\},\quad D:s\ne0,\quad 0\le\theta<2\pi
@wrong Check the requested form, every branch and the original domain before proceeding.
@why The c=0 branch gives pi/2 and 3pi/2, where sine is +/-1. The s=1/2 branch gives pi/6 and 5pi/6. All four have nonzero sine, lie in [0,2pi), and are distinct.
@step 70 | Which counts certify the complete original solution set?
@choice 72 | N=5,\quad R=0
@choice 73 | N=4,\quad R=1
@choice 71 | N=4,\quad R=0
@answer 71
@feedback 72 | The sine-zero endpoint 0 is forbidden in the original cotangent equation. The two surviving branches contribute exactly four angles.
@feedback 73 | All four substitutions satisfy the original equation, so no retained angle fails.
@after S=\left\{\frac{\pi}{6},\frac{\pi}{2},\frac{5\pi}{6},\frac{3\pi}{2}\right\},\quad N=4,\quad R=0,\quad D:s\ne0,\quad 0\le\theta<2\pi
@wrong Check the requested form, every branch and the original domain before proceeding.
@why At c=0, the left side 2c is zero and cotangent c/s is also zero because s=+/-1. At s=1/2, cotangent is c/(1/2)=2c, matching the original left side. The nonzero multiplier s and zero product prove completeness of these four angles.
@end
