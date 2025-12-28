'use strict';

const { draw, makeCylinder, makeBaseBox, makeBox } = replicad;

const defaultParams = {
  outerD: 48,
  moduleD: 44 + 0.6,
  axisDist: 45,
  glassH: 1.10,
  fullH: 11.90 - 1.10,
  innerH: 9.60 - 1.10,
  bearingOuterD: 22 + 0.4, // 608
  bearingInnerD: 8,
  bearingH: 7,
  skrewX: 15.75,
  skrewY: 11.50,
  connBlackX: 16.51,
  connBlackY: 0.3,
  connBlackW: 6,
  connBlackH: 11.5,
  connWhiteX: 7.20,
  connWhiteY: -13.50,
  connWhiteW: 5.5,
  connWhiteH: 5
};

const main = (r, $) => {
  let shape

  shape = draw() // main body
    .movePointerTo([$.outerD / 2, 0])
    .vLine($.axisDist/2)
    .bulgeArc(-$.outerD, 0, 1)
    .vLine(-$.axisDist)
    .bulgeArc($.outerD, 0, 1)
    .close()
    .sketchOnPlane()
    .extrude($.fullH)
    .chamfer(0.7)

  shape = shape.fuse(makeCylinder( // handle piedestal
    $.outerD / 2,
    $.fullH,
    [0, -$.axisDist / 2, $.glassH]
  ))

  shape = shape.cut(makeCylinder( // ESP32 module hole
    $.moduleD / 2,
    $.fullH,
    [0, $.axisDist / 2, $.fullH - $.innerH - 1.2]
  ))

  shape = shape.cut(makeCylinder( // ESP32 module glass clearence
    $.outerD / 2,
    $.fullH,
    [0, $.axisDist / 2, $.fullH]
  ))

  shape = shape.cut(makeCylinder( // outer bearing hole
    $.bearingOuterD / 2,
    $.bearingH,
    [0, -$.axisDist / 2, $.fullH + $.glassH - $.bearingH]
  ))

  shape = shape.cut(makeCylinder( // under bearing hole
    $.bearingOuterD / 2 - 1.5,
    $.fullH,
    [0, -$.axisDist / 2]
  ))

  shape = shape.cut( // sensor hole
    makeBaseBox(23.4, 23.4, 5)
      .chamfer(3, (e) => e.containsPoint([ 11.7, 0, 5]))
      .chamfer(3, (e) => e.containsPoint([-11.7, 0, 5]))
      .chamfer(3, (e) => e.containsPoint([0,  11.7, 5]))
      .chamfer(3, (e) => e.containsPoint([0, -11.7, 5]))
      .translate(0, -$.axisDist / 2)

  )

  shape = shape.cut( // wiring channel
    makeBaseBox(6, 25, 20)
      .translate(0, -7, -16)
      .chamfer(2.9)
  )

  // skrew holes
  shape = [[1, 1],[-1, 1],[-1, -1],[1, -1]].reduce((res, [x, y]) =>
    res
      .fuse (
        makeCylinder(3, $.fullH - $.innerH, [x * $.skrewX, y * $.skrewY + $.axisDist / 2, 0])
      )
      .cut(
      makeCylinder(2, 10, [x * $.skrewX, y * $.skrewY + $.axisDist / 2, -7.7])
        .chamfer(1.5)
    ), shape
  )

  // black connector holes
  shape = [-1, 1].reduce((res, x) =>
    res.cut(
      makeBaseBox($.connBlackW, $.connBlackH, 10)
        .translate(x * $.connBlackX, $.connBlackY + $.axisDist / 2)
    ), shape
  )

  // white connector holes
  shape = [-1, 1].reduce((res, x) =>
    res.cut(
      makeBaseBox($.connWhiteW, $.connWhiteH, 10)
        .translate(x * $.connWhiteX, $.connWhiteY + $.axisDist / 2)
    ), shape
  )

  // USB hole
  const usbHole = draw()
    .movePointerTo([0, -2])
    .hLine(3)
    .bulgeArc(0, 4, 1)
    .hLine(-6)
    .bulgeArc(0, -4, 1)
    .close()
    .sketchOnPlane('XZ')
    .extrude(10)

  shape = shape.cut(usbHole.translate([0, ($.outerD + $.axisDist) / 2, $.fullH - $.innerH + 2]));

  // button holes
  shape = [-1, 1].reduce((res, a) =>
    res.cut(
      makeCylinder(2, 30)
        .rotate(90, [], [1,0,0])
        .rotate((90 - 22.5) * a, [], [0,0,1])
        .translate(0, $.axisDist/2, 4.5)
    ), shape
  )

  return [
    {name: 'vernier-v0.4.0', shape, color: '#555', opacity: 1}
  ];
};
