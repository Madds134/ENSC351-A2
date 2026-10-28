# ENSC 351 Assignment 2: Light Dip Detector (`light_sampler`)

C program for the BeagleY-AI (aarch64, IP 192.168.7.2). The spec is `Assignment2_V1.pdf` in this folder. The student is Madden Wynans.
The course has an AI-use policy, so ask how much code to write versus explain or review. Don't assume.

## Build
- CMake with `hal/` (static lib, headers included as `"hal/xxx.h"`) and `app/`, using `file(GLOB src/*.c)`. Re-run configure after adding files.
- Cross build: `cmake -S . -B build -DCMAKE_C_COMPILER=aarch64-linux-gnu-gcc && cmake --build build`
- POST_BUILD copies the binary to `/home/madden/ensc351/public/myApps/light_sampler` (NFS-mounted on the target).
- `ENABLE_ASAN` option (default ON). Use OFF for sampling-rate tests and valgrind, and run valgrind only with it OFF.
- Flags: `-Wall -Werror -Wpedantic -Wextra`, C11. A host build fails at runtime (no `/dev/spidev0.0`). That's expected.
- Debug: `.vscode/launch.json` → gdbserver at `192.168.7.2:2001`, using `/usr/bin/gdb-multiarch`.

## Marking-guide rules that shape the code
- At least 4 modules with clear `.h` interfaces. Each `.h` needs a top description. Non-exported functions are `static`.
- Threads stay hidden inside modules: `init()`/`cleanup()`, and accessors lock a mutex.
- No magic numbers (use named `#define`/consts). No large or deeply nested functions. Use clear names.
- Existing style: `Module_function()` names, Doxygen `@brief` comments in headers, `isInitialized` guards.
- Valgrind memory errors cost up to 20 marks. Graceful `stop`: join threads, close sockets/fds, free memory.

## Current state (step 0 done)
- `hal/adc`: MCP3208 over SPI. `int ADC_read_channel(ch)` returns 0..`ADC_MAX_READING` (4095), or `ADC_READ_ERROR` (-1) on failure. Callers must skip error readings.
- `hal/led`: Assignment 1 status LEDs. It isn't needed for as2, but its `write_to_file` sysfs helper is reusable for PWM. Delete it after the PWM module exists.
- `app/src/main.c`: stub (ADC init/cleanup only).
- `app/src/periodTimer.c` + `app/include/periodTimer.h`: the prof's file, added unmodified. It already has `PERIOD_EVENT_SAMPLE_LIGHT` and `PERIOD_EVENT_MARK_SECOND`, and it builds clean with `-Werror`.
- `noworky/noworky.c`: the prof's buggy file, unmodified (step 11). The student finds the bug with gdb, so don't reveal it.
- Light sensor is on ADC CH0. `hal/light_sensor` (`bool LightSensor_readVoltage(double *volts)`, owns the ADC) is written. `hal/timing` (`Timing_sleepMs`) is shared.
- **Step 3 done (2026-10-28):** `app/sampler` (spec interface, ping-pong buffers, atomic stop flag) passed the board test with ASan OFF: 911 samples/s, avg period 1.098ms, max ≤1.18ms, clean exit. `main.c` is still the step 3 test loop.
- **PENDING (2026-10-28):** the step 1 board test hasn't run yet because the sensor wasn't available. Still need to confirm whether covering the sensor raises or lowers the voltage, before testing dips.
- **Course AI policy (pasted by the student on 2026-10-28):** the student does the high-level design and must be able to write all the code themselves. AI may write only a few lines at a time, never whole functions or files. Any function where AI wrote more than 3 lines needs a citation comment. Uncited or mostly-AI critical code means a zero or an academic-dishonesty report.
- I told the student about the policy risk. They chose to keep working the current way (I draft code in chat, they type it and ask for explanations) and to add AI declarations before submitting. Respect that choice and don't keep raising it. Keep the declarations accurate.
- **Track AI authorship for the declarations.** Whole functions drafted by AI so far: all of `light_sensor.c/.h`, all of `sampler.c` (the student wrote the accessor Doxygen), the test `main.c`, and `timing.c/.h` (student's as1 code moved into a module by AI). Not AI: `sampler.h` (spec), `periodTimer` (prof), `noworky.c` (prof), `adc`, `led`.
- Fresh git repo (no remote, nothing committed yet).

## Build plan (each step tested on the board before the next)
1. **Light sensor**: read the sensor's channel (from the course wiring guide) and convert to voltage (raw / 4095 × 3.3V). Determine whether covering the sensor raises or lowers the voltage, because the dip direction depends on it.
2. **periodTimer.c/.h** (provided on the course site): add an event enum such as `PERIOD_EVENT_SAMPLE_LIGHT`. Only for timing, never for storing samples.
3. **sampler** (interface from the PDF's `sampler.h`): a thread samples, marks the period event, and sleeps 1ms. Current buffer + history buffer behind a mutex. `Sampler_moveCurrentDataToHistory()` is called once a second. Exponential smoothing: the first sample sets avg, then `avg = 0.999*avg + 0.001*v`. Total count is `long long`. Needs ≥250 samples/s.
4. **dip_detector**: a pure function (samples, avg) → count. Triggers at 0.1V below the avg and re-arms at 0.07V below (0.03V hysteresis). Testable on the host.
5. **Once-per-second reporter thread**: move to history, count dips (store for UDP behind a mutex), get period stats, and print 2 fixed-width lines:
   `#Smpl/s = N  Flash @ NHz  avg = N.NNNV  dips = N  Smpl ms[min, max] avg A/N`
   followed by 10 evenly spaced samples `idx:value` (index `i*n/10`, or all of them if n < 10).
6. **udp_server** (port 12345, thread): `help`/`?`, `count`, `length`, `dips`, `history`, `stop`. Blank line repeats the last command, but is unknown if sent first. Unknown commands get an error message. `history`: voltages to 3 decimals, comma-separated, 10 per line, packets ≤1500 bytes, and never split a value across packets. `stop` signals main (flag/condvar) and replies "Program terminating.".
7. **hal/pwm**: sysfs per the course PWM guide. Set duty to 0 before changing the period (duty ≤ period). 50% duty. Disable at 0Hz.
8. **hal/rotary_encoder**: gpiod library (check the course guide for v1 vs v2), no external programs. Edge-event thread plus quadrature state machine, one count per detent (24 per full turn). Must work when spun fast in either direction.
9. **led_control**: starts at 10Hz, ±1Hz per detent, clamped to 0..500. Only rewrite PWM when the value changes.
10. **Shutdown and valgrind**: main inits everything, waits for stop, then cleans up in reverse order. Run valgrind with ASAN OFF.
11. **noworky** (separate): gdb text session using bt/list/up/print → `as2-gdb.txt`. VS Code screenshot → `as2-graphical.png`. One-word fix with a `// Bug was here:` comment.

## Submission
`as2.tgz` from the parent folder, without `build/`. Exclude this `CLAUDE.md` and the PDF (see README).
