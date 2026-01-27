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

  // lower piedestal
  shape = (makeCylinder($.bearingInnerD / 2 - 0.15, $.baseH).chamfer(0.5))

  // apper piedestal
  shape = shape.fuse(
    makeCylinder($.baseD / 2, $.baseH - $.bearingH - 2)
      .chamfer(2.9, (e) => e.containsPoint([$.baseD / 2, 0, 0]))
      .translateZ($.bearingH + 2)
  )

  // piedestal bottom hole
  shape = shape.cut(makeBaseBox(24, 32, $.baseH * 2).chamfer(4).translateZ(-$.baseH / 2))

  // skrew holes
  shape = [[1, 1],[-1, 1],[-1, -1],[1, -1]].reduce((res, [x, y]) =>
    res
      // .fuse (
      //   makeCylinder(3, $.fullH - $.innerH, [x * $.skrewX, y * $.skrewY, 0])
      // )
      .cut(makeCylinder(1, 20, [x * $.skrewX, y * $.skrewY, 0]))
      .cut(makeCylinder(3, $.baseH +1, [x * $.skrewX, y * $.skrewY, -2]).chamfer(2))
    , shape
  )

  // mount screws
  shape = [-1, 1].reduce((res, x) => res
    .cut(makeCylinder(1.5, 20, [x*16,0,0]))
    .cut(makeCylinder(5, 20, [x*16, 0, 2]).chamfer(4))
    , shape
  )

  // // black connector holes
  // shape = [-1, 1].reduce((res, x) =>
  //   res.cut(
  //     makeBaseBox($.connBlackW, $.connBlackH, 50)
  //       .translate(x * $.connBlackX, $.connBlackY)
  //   ), shape
  // )

  // adapter PCB
  shape = shape.cut(
    makeBaseBox($.connBlackX * 2 + $.connBlackW + 8, $.connBlackH + 4, 50)
      .translateZ(4)
  )

  // white connector holes
  shape = [-1, 1].reduce((res, x) =>
    res.cut(
      makeBaseBox($.connWhiteW, $.connWhiteH, 50)
        .translate(x * $.connWhiteX, $.connWhiteY)
    ), shape
  )

  return [
    {name: 'vernier-center-base-v0.2.0', shape, color: '#555', opacity: 1}
  ];
};
