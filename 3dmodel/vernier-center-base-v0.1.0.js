'use strict';

const { draw, makeCylinder, makeBaseBox, makeBox } = replicad;

const defaultParams = {
  baseH: 12,
  baseD: 44,

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
  shape = (makeCylinder($.bearingInnerD / 2 - 0.2, $.baseH).chamfer(0.5))

  // apper piedestal
  shape = shape.fuse(makeCylinder($.baseD / 2, $.baseH - $.bearingH - 2, [0, 0, $.bearingH + 2]).chamfer(0.5))

  // piedestal bottom hole
  shape = shape.cut(makeCylinder($.baseD / 2 - 6, $.baseH))

  // skrew holes
  shape = [[1, 1],[-1, 1],[-1, -1],[1, -1]].reduce((res, [x, y]) =>
    res
      // .fuse (
      //   makeCylinder(3, $.fullH - $.innerH, [x * $.skrewX, y * $.skrewY, 0])
      // )
      .cut(makeCylinder(1, 20, [x * $.skrewX, y * $.skrewY, 0]))
      .cut(makeCylinder(3, $.baseH - 1, [x * $.skrewX, y * $.skrewY, 0]))
    , shape
  )

  // black connector holes
  shape = [-1, 1].reduce((res, x) =>
    res.cut(
      makeBaseBox($.connBlackW, $.connBlackH, 50)
        .translate(x * $.connBlackX, $.connBlackY)
    ), shape
  )

  // white connector holes
  shape = [-1, 1].reduce((res, x) =>
    res.cut(
      makeBaseBox($.connWhiteW, $.connWhiteH, 50)
        .translate(x * $.connWhiteX, $.connWhiteY)
    ), shape
  )

  return [
    {name: 'vernier-center-base-v0.1.0', shape, color: '#555', opacity: 1}
  ];
};
