# Beneath design notes

## Coordinate model

Depth is measured downward from the surface. The submerged speaker sits at the deep-water side of the scene and the hydrophone at the shore side. `Rising Seabed` moves the hydrophone-side bottom toward the surface. Speaker and hydrophone depths are normalized to their local water columns, so neither can be dragged through the bottom.

The early acoustic field contains a direct path, a pressure-inverted surface path, and a bottom path found by mirroring the source across the seabed line. Continuous fractional-delay changes produce the pitch movement implied by motion.

## Deliberate musical departures

`Dream Physics` lengthens path times so aquarium-sized movements remain obvious. Seabed materials combine reflection gain and spectral damping. Procedural surf is intentionally more audible than a literal hydrophone model would predict.

Surface and bottom reflection gains are also weighted by the geometric proximity of both transducers to each boundary. `Distance Reverb` feeds a dark, highly damped room increasingly as the direct path grows. Air-stone proximity is part of the bubble generator rather than a cosmetic animation.

The former bubble-size parameter remains in the state layout for automation compatibility but is intentionally no longer exposed or used. The pump now produces a population of very short bright plops, while `Bubble Rate` raises the probability of a deep blorp from 5% to 40%. Reflection feedback is quiet, cross-coupled, material-dependent, and strongest when both transducers approach the seabed.

At maximum `Distance`, transmission filtering now reaches deliberately extreme material-dependent cutoffs: approximately 150 Hz over mud, 410 Hz over sand, 880 Hz over gravel, 1.65 kHz over rock, and 1.18 kHz over coral before additional Dream Physics darkening. Direct, surface, and bottom paths all participate, while the dark diffuse field becomes relatively more important.

## Rain and surface-drop system

The supplied Roland System-100M dripping-water patch suggests an event architecture rather than a sample:

1. White noise is sampled and held to choose a new irregular value for each drop.
2. The same event trigger fires two separate short envelopes.
3. One envelope sweeps a high-resonance filter, providing the droplet's pitch motion.
4. The other shapes the VCA, independently controlling its short amplitude contour.
5. Reverb changes the perceived setting from exposed rain to a resonant cave or enclosed pool.

Preview 18 implements `Rain Intensity` as event rate, clustering, drop-size distribution, and the transition toward a continuous heavy-rain noise bed; `Rain Volume` remains an independent immediate gain. Each discrete drop combines a sub-millisecond high-passed impact with a damped bubble resonance. Individual drops are distributed across the surface and pan relative to the hydrophone. Depth attenuates high resonances more strongly but never removes a storm completely. The temporary aquarium view shows falling strokes and small surface rings driven by the same Rain Intensity parameter.

Preview 19 begins the continuous bed at 35% intensity using a smoothstep fade, so it emerges slowly beneath the discrete events instead of appearing late. The bed is slightly louder at full scale. High drops are less common and substantially attenuated; the dominant lower population spans roughly 1.8–5.7 kHz. Resonance time constants are approximately halved, and every drop begins 8–25% above its target frequency before gliding down quickly. This replaces the exposed metal-tongue/music-box quality with a shorter downward droplet chirp embedded in the rain field.

Preview 20 preserves the perceptually successful downward chirp while constraining the complete high-drop family to 7.5–8.5 kHz. The narrower band removes the hardest crystal/mallet events above 8.5 kHz without changing the believable lower population or rain-bed balance.

Preview 21 gives only the continuous rain bed an irregular breathing contour. A slow 0.025–0.043 Hz sine is sampled at randomized 2.4–8.2 second intervals, perturbed slightly, and used as the next target for a long slew. The resulting bed wanders between approximately 78% and 122% of its knob-defined level without exposing a regular LFO cycle. Discrete drop timing and level are unchanged.

Preview 22 makes the final rain balance trim before graphic work. Discrete drop gain is reduced by approximately 1.4 dB at all depths. Their depth attenuation is also strengthened: lower resonances retain about 58% rather than 70% of their surface level at the seabed, while the highest drops are reduced more strongly. The continuous bed, its depth-dependent cutoff, irregular motion, event timing, and drop contours remain unchanged.

Preview 23 begins the graphic phase with a Game & Watch aquarium frame concept. A brushed champagne casing and layered dark bezel surround a muted procedural underwater plate. The fixed boat and left-side plants are rendered as faded printed colour artwork; waves, rain, seabed, cables, speaker, hydrophone, air stone, and bubbles remain dark LCD-like elements. Wave motion uses three stepped amplitudes and quantized animation frames, while the rising right-hand seabed is now an uneven black contour rather than a solid coloured wedge. All existing scene dragging and audio controls remain functional.

Preview 24 applies the first visual-QA corrections to the frame concept: extreme scene positions are inset just enough to keep the speaker, hydrophone, and air stone visible inside the LCD window, and control readouts are limited to two decimal places.

Preview 25 replaces the procedural plate with the supplied underwater photograph embedded in the binary. The rising seabed is now a fully opaque near-black LCD mass. Printed plants on the left grow almost to the surface and use individual green or brown gradients that darken toward their roots. Speaker and hydrophone cables are thinner interrupted lines, including the current-displaced cable positions. The boat, stepped waves, rain, and temporary control panel are otherwise unchanged.

Preview 26 applies the visual-QA follow-up: two-decimal readout formatting is now applied after parameter attachments establish each slider range, and equipment captions are clamped inside the LCD window at extreme horizontal positions.

Preview 27 crops the photographic ocean plate precisely at the wave line. A subdued printed sky occupies the rain and boat region above it, while the photograph's bright upper edge begins at the water surface and extends only through the underwater region.

Preview 28 overrides JUCE's parameter-provided slider text formatter so all temporary knob readouts are consistently rendered with two decimal places.

Preview 29 explicitly refreshes each slider's cached text after installing the two-decimal formatter, ensuring the initial visible readouts match subsequent parameter changes.

Preview 30 reorganizes the interface around the aquarium. Four red rubber buttons flank the screen for Waves, Rain, Marine Life, and Bubbles; each button toggles its layer and selects a compact contextual control row. The Distance knob is removed from the visible interface and distance, speaker depth, and hydrophone depth appear as two-digit segmented LCD readouts in the sky. Input, Output, and Mix remain permanently available on the lower front panel, while water depth, rising seabed, current, dream physics, distance reverb, seabed material, and pause move into a Settings drawer. The printed plants now switch among discrete current-driven sway poses, retaining the hybrid painted/LCD character.

Preview 31 completes the rubber-control treatment. The environment buttons move down so their lower row aligns with the screen, freeing the upper side rails for vertical red-fill Input and Output faders. Settings becomes a black-ringed red rubber pill directly beneath the screen. All rotary controls use red rubber faces, black index dots, numeric readouts, and category-coloured surrounds. Context and Settings panels dismiss when the aquarium or unused casing is clicked. Waves and the visible seabed contour are rebuilt from short bevel-ended LCD cells; the seabed remains an opaque solid mass below its segmented edge.

Preview 32 adds a restrained persistent red backlight to enabled environment buttons. This active-state halo remains visible when contextual controls are dismissed, while the pale rectangular surround continues to indicate only the currently open control panel.

Preview 33 starts with environmental context controls dismissed, tightens and thickens the bevel-ended wave cells so they read as one segmented contour rather than isolated beads, and raises the permanent Mix control for clearer lower-frame spacing.

Preview 34 corrects clipping in both custom control families: side buttons shift their rubber face and halo downward within a taller safe area, while rotary faces are inset from JUCE's drawable bounds. Mix becomes the same visual size as the four environment buttons and moves to the lower right in line with the Settings pill, leaving the lower-left casing open for a future angled printed title illustration. Settings moves fully below the aquarium frame. The opaque seabed again changes material colour: dark umber mud, ochre sand, slate gravel, ink-green rock, and muted red-brown coral, each retaining a darker segmented LCD edge.

Preview 35 gives every seabed material its own drawn overlay inside the opaque mass: flowing sediment grooves for mud, short repeating ripples for sand, angular pebble cells for gravel, fractured branches for rock, and sparse organic branching marks for coral. Textures inherit a lighter translucent version of the selected material colour and never cross above the segmented seabed edge.

Preview 36 clips all material textures against the complete seabed path, preventing Gravel and Rock marks from escaping the lower LCD frame. The aquarium and side hardware move down 12 pixels to add breathing room above the display, and the small upper-left text title is removed. The supplied transparent Beneath wave logo is embedded as faded printed artwork across the lower-left casing, rotated 1.6 degrees upward toward the right; contextual and Settings controls remain child components and therefore appear naturally above the printed logo.

Preview 37 improves aquarium legibility by moving the Speaker and Hydrophone captions above their symbols, brightening them, and enlarging the three segmented readouts. The boat mast now hides a feedback easter egg: hovering reveals the Jolly Roger, and vertical dragging hoists it while morphing the normal reflection return toward a protected near-unity cross-feedback loop. A 110 Hz low cut, 7.8 kHz damping, soft saturation, and a hard return ceiling prevent DC, infrasonic, ultrasonic, and runaway peak accumulation.

Preview 38 caps Pirate Feedback at 90%, retaining a deliberately wild upper edge without exposing the self-sustaining former endpoint. The mast interaction uses a much larger invisible target and a 96-pixel relative drag throw for precise adjustment while keeping the complete range reachable in one upward gesture; timer-based hover tracking makes flag discovery reliable across hosts. Beneath intentionally ships without named factory presets so each user develops settings around their own source material.

Release 1.0.1 restores the noise-machine behaviour without changing the flag's ominous 90% ceiling. The control remains linear through 85%; its final five displayed points bend quadratically toward the original full loop amount, so maximum sail again reaches the self-sustaining endpoint. Direct and reflected input, waves, whales, rain, and bubbles all enter the return before its low cut, damping, saturation, and hard ceiling. The undocumented remaining ten percent is reserved for failures of gravity and possible visits from Pazuzu.

Preview 37 moves the seabed material selector into a sixth Settings-column position directly below the Settings pill and above Mix. All numeric slider readouts now use an opaque light-casing plate with a subtle dark outline, keeping values legible over the printed logo and other artwork.

Preview 38 moves Pause Environment out of the printed logo area and into the right-hand utility column, directly below the seabed selector and immediately above Mix.

The same two-envelope resonator may later create quiet surface ticks when synthesized pump bubbles reach the waterline, but those arrivals should remain a separate layer from the underwater bubble body.

## Motion model

`Current` physically displaces both the speaker and hydrophone from their manually placed resting positions. The hydrophone has greater drag than the speaker, cable deflection lifts each object slightly, and the resulting direct, surface, and seabed path lengths produce the delay movement. Two incommensurate components remain, but their rate, strength, and directional bias now follow a slowly interpolated random ebb-and-flow state. The same motion is shown in the aquarium; faint dashed cables and resting-position marks preserve automation legibility.

Beach waves are discrete events. Intensity maps logarithmically from an approximately 15-second interval to a 1-second interval, while event duration contracts from about 4.8 seconds to 1.05 seconds. The pre-break rise is low-passed between roughly 105 and 720 Hz; the crash can brighten as far as 6.5 kHz at maximum intensity. Rising Seabed always changes reflection geometry, but generated surf remains silent for its first 20% and then fades in with a curved response.

A continuously running low-passed pink-noise rumble sits underneath the discrete events whenever Wave Intensity is non-zero. Its nominal level rises only from 10% to 40% across the intensity range. Two very slow incommensurate phases move its level, cutoff, and stereo balance, and each scheduled wave swells out of and recedes into this bed. The crash envelope and spectral opening are smoothed across the breaking point to avoid a hard step.

The complete wave generator is attenuated by a further 0.22 output scale so it remains environmental background. Wave timing varies from roughly 55% to 165% of the nominal interval. Crest levels loosely build through seven-wave families: the seventh has a strong probability of becoming a featured wave, while rare earlier waves may also take that role. Featured waves are followed by a 130% to 205% interval, creating the longer recovery after a large swell.

Hydrophone depth imposes a logarithmic ceiling on every generated wave component, including the pink-noise rumble and breaking wash. The ceiling remains 12 kHz at the surface and approximately 90 Hz at the bottom, but Preview 9 uses a quintic endpoint-smoothed openness curve. A tiny lift from the seabed therefore remains near 90 Hz, while most of the audible bloom is spread through the middle half of the water column.

Distance filtering uses a gentler curve through the first half of its range. Above 50%, a dedicated basement-murk curve increasingly dominates. Its final cutoff depends on water depth plus both transducer depths and reaches approximately 150 Hz when all are maximally deep, regardless of reflective seabed material.

When Distance exceeds 65% and the combined water/transducer depth exceeds 55%, up to 0.075 additional feedback is introduced. This abyss-only reserve is independent of seabed reflectivity and allows heavily filtered returns to roll along even over absorptive material without raising ordinary-scene feedback.

Speaker, hydrophone, and air stone horizontal positions now span the complete scene. Each transducer's vertical range is calculated against the seabed depth at its own horizontal position. Dragging the speaker or hydrophone horizontally also updates the Distance macro from their separation; the objects may cross or occupy the same side of the aquarium.

## Next audition questions

1. Is the surface-path polarity musically useful or merely hollow?
2. Do source and hydrophone depth changes produce sufficiently distinct scenes?
3. Are sand and mud too quiet to be enjoyable?
4. Does a sloping seabed sound like approaching shore before the surf generator is heard?
5. Does Preview 17's redesigned air stone read as rounded bubbles at low pressure without recovering the former snare-like noise when close-miked?
6. Does the reflection feedback audibly recede without becoming a conventional echo effect?
7. Is the dark distance reverb roomy enough without masking the three early paths?
8. Does Preview 17's capped full-turn marine control range preserve useful adjustment throughout both knobs?

## Marine life prototype

Preview 10 introduced a deterministic procedural `Whale Song` layer based on the earlier ASR-10 experiment: a single high-to-low slide is copied four times with progressively increasing predelay and spacing, small detuning, stereo spread, and a dark reverb send. Preview 11 removed the note-like middle register. Preview 12 lowers the fundamental drone band again to approximately 32–400 Hz and raises the separate high band to roughly 7–10.5 kHz. Upward gestures are now brief strokes, usually well below one second; low descending calls last as long as 9.5 seconds and gently sway around their overall fall. Some long drones carry restrained second, third, and fourth partials, creating parallel compound bands without turning the fundamental into a bright saw. The source remains a gently phase-bent sine. Overall whale gain is reduced by roughly 4 dB, with the high family quieter again, so the layer remains behind the processed input.

Preview 13 compresses the four low-band copies into a single 30–150 ms cluster. They now fuse as one thick, compound source rather than four audible echoes. High-band events use three to seven copies separated by approximately 42–110 ms, turning the successful short upward blips into compact, irregular dolphin-like squeak bursts. The burst remains part of the provisional Whale Song control until its identity and balance justify a separate marine-life control.

Preview 14 biases the low four-copy cluster toward 10–30 ms, with a progressively rarer tail reaching 150 ms. Low descending calls begin at logarithmically distributed positions throughout 40–400 Hz but fall by at most three semitones; most fall less, and their internal sway is reduced to well under a semitone. Roughly one quarter instead hold an almost steady drone and make a fast 0.8–3 semitone lift near the end. High squeak bursts retain Preview 13's timing.

Preview 15 organizes each high event into two or three repeated phrases of three to five blips. Blip duration is unchanged, while phrase gaps extend the complete chatter and a randomized ±0.45–1.8 semitone drift gently raises or lowers successive repetitions. `Whale Song` now principally controls event activity; the appended `Whale Volume` parameter applies immediate gain to both active and future calls. Its default is 75%. Low calls are attenuated relative to Preview 14 to compensate for their duration and compound harmonics, while the high family is proportionally less recessed.

Preview 16 assigns every marine phrase a fixed deterministic source beyond the visible aquarium, biased toward the side opposite the hydrophone. The received image is normally close to a 60/40 power balance and is clamped well short of hard panning. Manual placement plus current-driven hydrophone displacement creates mild pan and level parallax without modifying call pitch. Greater hydrophone depth slightly favours the low drones and attenuates high squeaks by no more than 18%, providing a small vertical listening-position cue.

Preview 17 caps the full-turn `Whale Song` activity range at 75% of the original scheduler scale and `Whale Volume` at 0.5; the latter defaults to 0.375, preserving its three-quarter knob position. Ordinary air-stone bubbles no longer use a per-sample noise transient or five-sample attack. Their resonant body is lowered from the former 260–1850 Hz range to 220–880 Hz, lengthened, given a 2.2 ms smooth attack, and glides upward more slowly. Low pump pressure also favours the 48–126 Hz deep blorp population, removing the close-miked early-drum-machine snare character.

## Deferred, non-blocking ideas

The acoustic prototype is complete enough to begin final graphic design. These remain optional later expansions rather than blockers:

1. A recordable hydrophone gesture path for looping circular dives, pulls, releases, and other playable delay movement.
2. More elastic cable rendering with downstream bow, inertia, and flutter around the existing physical transducer positions.
3. Surface-arrival ticks when pump bubbles reach the waterline.

Wave Volume remains fixed internally at 45%; its hidden parameter slot is retained for state and automation compatibility.

