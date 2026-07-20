'use strict';

const { draw, makeCylinder, makeBaseBox, makeBox } = replicad;

const defaultParams = {
  baseH: 12,
  baseD: 46,

  bearingOuterD: 52, // 6808
  bearingInnerD: 40,
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

  shape = (
    [[1,1], [-1,1], [-1,-1], [1,-1]].reduce((res, [x, y]) => res
      .fillet(8, (e) => e.containsPoint([x * 40, y * 40, 1])),
      makeBaseBox(80,80,3)
    )
  )

  // bearing spacer ring
  shape = shape.fuse(
    makeCylinder($.bearingInnerD / 2 + 3, 5)
      .cut(makeCylinder($.bearingInnerD / 2 + 0.1, 5))
  )

  // piedestal bottom hole
  shape = shape.cut(makeBaseBox(24, 32, $.baseH * 2).chamfer(4).translateZ(-$.baseH / 2))

  // mount screws
  shape = [-1, 1].reduce((res, x) => res
    .cut(makeCylinder(1.5, 20, [x*16, 0, 0]))
    .cut(makeCylinder(5, 10, [x*16, 0, -8]).chamfer(4))
    , shape
  )

  return [
    {name: 'vernier-center-panel-v0.1.0', shape, color: '#555', opacity: 1}
  ];
};
