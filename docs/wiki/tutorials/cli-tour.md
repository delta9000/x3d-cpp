---
title: Tour the x3d command line
summary: Convert, validate, simulate and export one small scene with the x3d tool, without writing code.
tags: [tutorial, cli]
updated: 2026-11-14
related:
  - ../reference/subsystems/cli-suite.md
  - first-scene.md
---

# Tour the x3d command line

In this tutorial you take one small animated scene through every `x3d`
subcommand: convert it to another encoding, validate it, watch its behavior
run, and export its geometry. You write no code.

You need a built checkout. If you don't have one, run step 1 of
[Load, run and extract your first scene](first-scene.md); the build leaves the
tool at `build/x3d`.

## 1. Write the scene

Make a working directory and save this as `spin.x3d`:

```xml
<?xml version="1.0" encoding="UTF-8"?>
<X3D profile="Interchange" version="4.0">
  <Scene>
    <Transform DEF="Spin">
      <Shape>
        <Appearance><Material diffuseColor="0.8 0.2 0.2"/></Appearance>
        <Box size="2 1 1"/>
      </Shape>
    </Transform>
    <TimeSensor DEF="Clock" cycleInterval="4" loop="true"/>
    <OrientationInterpolator DEF="Turn" key="0 0.5 1"
        keyValue="0 1 0 0  0 1 0 3.14159  0 1 0 6.28318"/>
    <ROUTE fromNode="Clock" fromField="fraction_changed" toNode="Turn" toField="set_fraction"/>
    <ROUTE fromNode="Turn" fromField="value_changed" toNode="Spin" toField="rotation"/>
  </Scene>
</X3D>
```

In the commands below, `x3d` stands for the path to your built tool, for
example `/path/to/x3d-cpp/build/x3d`.

## 2. Convert it to ClassicVRML

```sh
x3d convert spin.x3d -o spin.x3dv -f vrml
```

The command prints nothing on success. Open `spin.x3dv`:

```text
#X3D V4.0 utf8
PROFILE Interchange

DEF Spin Transform {
  children [
    Shape {
      appearance Appearance {
        material Material {
          diffuseColor 0.8 0.2 0.2
        }
      }
      geometry Box {
        size 2 1 1
      }
    }
  ]
}

DEF Clock TimeSensor {
  cycleInterval 4
  loop TRUE
}

DEF Turn OrientationInterpolator {
  key [ 0 0.5 1 ]
  keyValue [ 0 1 0 0 0 1 0 3.14159 0 1 0 6.28318 ]
}

ROUTE Clock.fraction_changed TO Turn.set_fraction
ROUTE Turn.value_changed TO Spin.rotation
```

It is the same scene in another encoding. `-f xml` and `-f json` select the
other two writers.

## 3. Validate it

```sh
x3d validate spin.x3d
```

```text
file:          spin.x3d
declared:      Interchange 4.0
profile fit:   Interchange
valid:         yes

No diagnostics found.
```

Now break it. Copy the file to `broken.x3d` and change `toNode="Spin"` in the
last ROUTE to `toNode="Spinn"`. Validate the copy:

```sh
x3d validate broken.x3d
```

```text
file:          broken.x3d
declared:      Interchange 4.0
profile fit:   Interchange
valid:         no

[route]
  error: ROUTE Turn.value_changed TO Spinn.rotation: no node is DEF'd as 'Spinn'

Total: 1 diagnostic(s)
```

The exit status is 3 when there are diagnostics and 0 when there are none, so
a script can branch on it. Add `--json` for structured output.

## 4. Run the behavior

`sim` ticks the scene and prints the fields that changed. Run four ticks at two
ticks per second and watch one field:

```sh
x3d sim spin.x3d --fps 2 --ticks 4 --watch Spin.rotation
```

```text
tick 0 (t=0.0000):
  Spin.rotation = 0 1 0 0
tick 1 (t=0.5000):
  Spin.rotation = 0 1 0 0.7853975
tick 2 (t=1.0000):
  Spin.rotation = 0 1 0 1.570795
tick 3 (t=1.5000):
  Spin.rotation = 0 1 0 2.3561926
```

The box turns by an eighth of a turn every half second, which matches the
four-second cycle. Leave out `--watch` to see every changed field, including
the TimeSensor outputs that drive the turn.

## 5. Export the geometry

```sh
x3d extract spin.x3d -o spin.stl
```

```text
extract: 12 triangle(s) written to 'spin.stl'
```

A box is 12 triangles. Ask for glTF instead and the export keeps the material
and the animation:

```sh
x3d extract spin.x3d -o spin.glb
```

```text
extract: 1 node(s), 1 mesh(es), 1 material(s), 0 texture(s), 0 light(s), 0 camera(s), 1 animation(s), 12 triangle(s) written to 'spin.glb'
```

Open `spin.glb` in any glTF viewer to see the box turn.

## 6. Write the canonical form

```sh
x3d canonicalize spin.x3d -o spin.canon.x3d
```

The output is the same scene in X3D Canonical Form: a fixed DOCTYPE,
single-quoted attributes and a fixed attribute order, which gives diff-based
tools a deterministic form to compare.

## What you did

You converted, validated, simulated and exported a scene with five
subcommands. Each one is a thin layer over the SDK calls from
[the first tutorial](first-scene.md).

## Next steps

- [CLI suite](../reference/subsystems/cli-suite.md) lists every option, the
  diagnostic categories and the loss rules for `convert`.
- [Add a CLI command](../how-to/add-a-cli-command.md) shows how to add a sixth.
