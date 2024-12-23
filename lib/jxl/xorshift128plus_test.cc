// Copyright (c) the JPEG XL Project Authors. All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include <jxl/memory_manager.h>

#include <algorithm>
#include <cstdint>
#include <vector>

#include "lib/jxl/memory_manager_internal.h"
#include "lib/jxl/test_memory_manager.h"

#undef HWY_TARGET_INCLUDE
#define HWY_TARGET_INCLUDE "lib/jxl/xorshift128plus_test.cc"
#include <hwy/foreach_target.h>
#include <hwy/highway.h>
#include <hwy/tests/hwy_gtest.h>

#include "lib/jxl/base/data_parallel.h"
#include "lib/jxl/test_utils.h"
#include "lib/jxl/testing.h"
#include "lib/jxl/xorshift128plus-inl.h"

HWY_BEFORE_NAMESPACE();
namespace jxl {
namespace HWY_NAMESPACE {

// These templates are not found via ADL.
using hwy::HWY_NAMESPACE::Or;
using hwy::HWY_NAMESPACE::ShiftRight;
using hwy::HWY_NAMESPACE::Sub;

// Define to nonzero in order to print the (new) golden outputs.
#define PRINT_RESULTS 0

const size_t kVectors = 64;

#if PRINT_RESULTS

template <int kNumLanes>
void Print(const uint64_t (&result)[kNumLanes]) {
  printf("{ ");
  for (int i = 0; i < kNumLanes; ++i) {
    if (i != 0) {
      printf(", ");
    }
    printf("0x%016llXull", result[i]);
  }
  printf("},\n");
}

#else  // PRINT_RESULTS

const uint64_t kExpected[kVectors][Xorshift128Plus::N] = {
    {0x6E901576D477CBB1ull, 0xE9E53789195DA2A2ull, 0xB681F6DDA5E0AE99ull,
     0x8EFD18CE21FD6896ull, 0xA898A80DF75CF532ull, 0x50CEB2C9E2DE7E32ull,
     0x3CA7C2FEB25C0DD0ull, 0xA4D0866B80B4D836ull},
    {0x8CD6A1E6233D3A26ull, 0x3D4603ADE98B112Dull, 0xDC427AF674019E36ull,
     0xE28B4D230705AC53ull, 0x7297E9BBA88783DDull, 0x34D3D23CFCD9B41Aull,
     0x5A223615ADBE96B8ull, 0xE5EB529027CFBD01ull},
    {0xC1894CF00DFAC6A2ull, 0x18EDF8AE9085E404ull, 0x8E936625296B4CCDull,
     0x31971EF3A14A899Bull, 0xBE87535FCE0BF26Aull, 0x576F7A752BC6649Full,
     0xA44CBADCE0C6B937ull, 0x3DBA819BB17A353Aull},
    {0x27CE38DFCC1C5EB6ull, 0x920BEB5606340256ull, 0x3986CBC40C9AFC2Cull,
     0xE22BCB3EEB1E191Eull, 0x6E1FCDD3602A8FBAull, 0x052CB044E5415A29ull,
     0x46266646EFB9ECD7ull, 0x8F44914618D29335ull},
    {0xDD30AEDF72A362C5ull, 0xBC1D824E16BB98F4ull, 0x9EA6009C2AA3D2F1ull,
     0xF65C0FBBE17AF081ull, 0x22424D06A8738991ull, 0x8A62763F2B7611D2ull,
     0x2F3E89F722637939ull, 0x84D338BEF50AFD50ull},
    {0x00F46494898E2B0Bull, 0x81239DC4FB8E8003ull, 0x414AD93EC5773FE7ull,
     0x791473C450E4110Full, 0x87F127BF68C959ACull, 0x6429282D695EF67Bull,
     0x661082E11546CBA8ull, 0x5815D53FA5436BFDull},
    {0xB3DEADAB9BE6E0F9ull, 0xAA1B7B8F7CED0202ull, 0x4C5ED437699D279Eull,
     0xA4471727F1CB39D3ull, 0xE439DA193F802F70ull, 0xF89401BB04FA6493ull,
     0x3B08045A4FE898BAull, 0x32137BFE98227950ull},
    {0xFBAE4A092897FEF3ull, 0x0639F6CE56E71C8Eull, 0xF0AD6465C07F0C1Eull,
     0xFF8E28563361DCE5ull, 0xC2013DB7F86BC6B9ull, 0x8EFCC0503330102Full,
     0x3F6B767EA5C4DA40ull, 0xB9864B950B2232E1ull},
    {0x76EB58DE8E5EC22Aull, 0x9BBBF49A18B32F4Full, 0xC8405F02B2B2FAB9ull,
     0xC3E122A5F146BC34ull, 0xC90BB046660F5765ull, 0xB933981310DBECCFull,
     0x5A2A7BFC9126FD1Cull, 0x8BB388C94DF87901ull},
    {0x753EB89AD63EF3C3ull, 0xF24AAF40C89D65ADull, 0x23F68931C1A6AA6Dull,
     0xF47E79BF702C6DD0ull, 0xA3AD113244EE7EAEull, 0xD42CBEA28F793DC3ull,
     0xD896FCF1820F497Cull, 0x042B86D2818948C1ull},
    {0x8F2A4FC5A4265763ull, 0xEC499E6F95EAA10Cull, 0xE3786D4ECCD0DEB5ull,
     0xC725C53D3AC4CC43ull, 0x065A4ACBBF83610Eull, 0x35C61C9FEF167129ull,
     0x7B720AEAA7D70048ull, 0x14206B841377D039ull},
    {0xAD27D78BF96055F6ull, 0x5F43B20FF47ADCD4ull, 0xE184C2401E2BF71Eull,
     0x30B263D78990045Dull, 0xC22F00EBFF9BA201ull, 0xAE7F86522B53A562ull,
     0x2853312BC039F0A4ull, 0x868D619E6549C3C8ull},
    {0xFD5493D8AE9A8371ull, 0x773D5E224DF61B3Bull, 0x5377C54FBB1A8280ull,
     0xCAD4DE3B8265CAFAull, 0xCDF3F19C91EBD5F6ull, 0xC8EA0F182D73BD78ull,
     0x220502D593433FF1ull, 0xB81205E612DC31B1ull},
    {0x8F32A39EAEDA4C70ull, 0x1D4B0914AA4DAC7Full, 0x56EF1570F3A8B405ull,
     0x29812CB17404A592ull, 0x97A2AAF69CAE90F2ull, 0x12BF5E02778BBFE5ull,
     0x9D4B55AD42A05FD2ull, 0x06C2BAB5E6086620ull},
    {0x8DB4B9648302B253ull, 0xD756AD9E3AEA12C7ull, 0x68709B7F11D4B188ull,
     0x7CC299DDCD707A4Bull, 0x97B860C370A7661Dull, 0xCECD314FC20E64F5ull,
     0x55F412CDFB4C7EC3ull, 0x55EE97591193B525ull},
    {0xCF70F3ACA96E6254ull, 0x022FEDECA2E09F46ull, 0x686823DB60AE1ECFull,
     0xFD36190D3739830Eull, 0x74E1C09027F68120ull, 0xB5883A835C093842ull,
     0x93E1EFB927E9E4E3ull, 0xB2721E249D7E5EBEull},
    {0x69B6E21C44188CB8ull, 0x5D6CFB853655A7AAull, 0x3E001A0B425A66DCull,
     0x8C57451103A5138Full, 0x7BF8B4BE18EAB402ull, 0x494102EB8761A365ull,
     0xB33796A9F6A81F0Eull, 0x10005AB3BCCFD960ull},
    {0xB2CF25740AE965DCull, 0x6F7C1DF7EF53D670ull, 0x648DD6087AC2251Eull,
     0x040955D9851D487Dull, 0xBD550FC7E21A7F66ull, 0x57408F484DEB3AB5ull,
     0x481E24C150B506C1ull, 0x72C0C3EAF91A40D6ull},
    {0x1997A481858A5D39ull, 0x539718F4BEF50DC1ull, 0x2EC4DC4787E7E368ull,
     0xFF1CE78879419845ull, 0xE219A93DD6F6DD30ull, 0x85328618D02FEC1Aull,
     0xC86E02D969181B20ull, 0xEBEC8CD8BBA34E6Eull},
    {0x28B55088A16CE947ull, 0xDD25AC11E6350195ull, 0xBD1F176694257B1Cull,
     0x09459CCF9FCC9402ull, 0xF8047341E386C4E4ull, 0x7E8E9A9AD984C6C0ull,
     0xA4661E95062AA092ull, 0x70A9947005ED1152ull},
    {0x4C01CF75DBE98CCDull, 0x0BA076CDFC7373B9ull, 0x6C5E7A004B57FB59ull,
     0x336B82297FD3BC56ull, 0x7990C0BE74E8D60Full, 0xF0275CC00EC5C8C8ull,
     0x6CF29E682DFAD2E9ull, 0xFA4361524BD95D72ull},
    {0x631D2A19FF62F018ull, 0x41C43863B985B3FAull, 0xE052B2267038EFD9ull,
     0xE2A535FAC575F430ull, 0xE004EEA90B1FF5B8ull, 0x42DFE2CA692A1F26ull,
     0x90FB0BFC9A189ECCull, 0x4484102BD3536BD0ull},
    {0xD027134E9ACCA5A5ull, 0xBBAB4F966D476A9Bull, 0x713794A96E03D693ull,
     0x9F6335E6B94CD44Aull, 0xC5090C80E7471617ull, 0x6D9C1B0C87B58E33ull,
     0x1969CE82E31185A5ull, 0x2099B97E87754EBEull},
    {0x60EBAF4ED934350Full, 0xC26FBF0BA5E6ECFFull, 0x9E54150F0312EC57ull,
     0x0973B48364ED0041ull, 0x800A523241426CFCull, 0x03AB5EC055F75989ull,
     0x8CF315935DEEB40Aull, 0x83D3FC0190BD1409ull},
    {0x26D35394CF720A51ull, 0xCE9EAA15243CBAFEull, 0xE2B45FBAF21B29E0ull,
     0xDB92E98EDE73F9E0ull, 0x79B16F5101C26387ull, 0x1AC15959DE88C86Full,
     0x387633AEC6D6A580ull, 0xA6FC05807BFC5EB8ull},
    {0x2D26C8E47C6BADA9ull, 0x820E6EC832D52D73ull, 0xB8432C3E0ED0EE5Bull,
     0x0F84B3C4063AAA87ull, 0xF393E4366854F651ull, 0x749E1B4D2366A567ull,
     0x805EACA43480D004ull, 0x244EBF3AA54400A5ull},
    {0xBFDC3763AA79F75Aull, 0x9E3A74CC751F41DBull, 0xF401302A149DBC55ull,
     0x6B25F7973D7BF7BCull, 0x13371D34FDBC3DAEull, 0xC5E1998C8F484DCDull,
     0x7031B8AE5C364464ull, 0x3847F0C4F3DA2C25ull},
    {0x24C6387D2C0F1225ull, 0x77CCE960255C67A4ull, 0x21A0947E497B10EBull,
     0xBB5DB73A825A9D7Eull, 0x26294A41999E553Dull, 0x3953E0089F87D925ull,
     0x3DAE6E5D4E5EAAFEull, 0x74B545460341A7AAull},
    {0x710E5EB08A7DB820ull, 0x7E43C4E77CAEA025ull, 0xD4C91529C8B060C1ull,
     0x09AE26D8A7B0CA29ull, 0xAB9F356BB360A772ull, 0xB68834A25F19F6E9ull,
     0x79B8D9894C5734E2ull, 0xC6847E7C8FFD265Full},
    {0x10C4BCB06A5111E6ull, 0x57CB50955B6A2516ull, 0xEF53C87798B6995Full,
     0xAB38E15BBD8D0197ull, 0xA51C6106EFF73C93ull, 0x83D7F0E2270A7134ull,
     0x0923FD330397FCE5ull, 0xF9DE54EDFE58FB45ull},
    {0x07D44833ACCD1A94ull, 0xAAD3C9E945E2F9F3ull, 0xABF4C879B876AA37ull,
     0xF29C69A21B301619ull, 0x2DDCE959111C788Bull, 0x7CEDB48F8AC1729Bull,
     0x93F3BA9A02B659BEull, 0xF20A87FF17933CBEull},
    {0x8E96EBE93180CFE6ull, 0x94CAA12873937079ull, 0x05F613D9380D4189ull,
     0xBCAB40C1DC79F38Aull, 0x0AD8907B7C61D19Eull, 0x88534E189D103910ull,
     0x2DB2FAABA160AB8Full, 0xA070E7506B06F15Cull},
    {0x6FB1FCDAFFEF87A9ull, 0xE735CF25337A090Dull, 0x172C6EDCEFEF1825ull,
     0x76957EA49EF0542Dull, 0x819BF4CD250F7C49ull, 0xD6FF23E4AD00C4D4ull,
     0xE79673C1EC358FF0ull, 0xAC9C048144337938ull},
    {0x4C5387FF258B3AF4ull, 0xEDB68FAEC2CB1AA3ull, 0x02A624E67B4E1DA4ull,
     0x5C44797A38E08AF2ull, 0x36546A70E9411B4Bull, 0x47C17B24D2FD9675ull,
     0x101957AAA020CA26ull, 0x47A1619D4779F122ull},
    {0xF84B8BCDC92D9A3Cull, 0x951D7D2C74B3066Bull, 0x7AC287C06EDDD9B2ull,
     0x4C38FC476608D38Full, 0x224D793B19CB4BCDull, 0x835A255899BF1A41ull,
     0x4AD250E9F62DB4ABull, 0xD9B44F4B58781096ull},
    {0xABBAF99A8EB5C6B8ull, 0xFB568E900D3A9F56ull, 0x11EDF63D23C5DF11ull,
     0xA9C3011D3FA7C5A8ull, 0xAEDD3CF11AFFF725ull, 0xABCA472B5F1EDD6Bull,
     0x0600B6BB5D879804ull, 0xDB4DE007F22191A0ull},
    {0xD76CC9EFF0CE9392ull, 0xF5E0A772B59BA49Aull, 0x7D1AE1ED0C1261B5ull,
     0x79224A33B5EA4F4Aull, 0x6DD825D80C40EA60ull, 0x47FC8E747E51C953ull,
     0x695C05F72888BF98ull, 0x1A012428440B9015ull},
    {0xD754DD61F9B772BFull, 0xC4A2FCF4C0F9D4EBull, 0x461167CDF67A24A2ull,
     0x434748490EBCB9D4ull, 0x274DD9CDCA5781DEull, 0x36BAC63BA9A85209ull,
     0x30324DAFDA36B70Full, 0x337570DB4FE6DAB3ull},
    {0xF46CBDD57C551546ull, 0x8E02507E676DA3E3ull, 0xD826245A8C15406Dull,
     0xDFB38A5B71113B72ull, 0x5EA38454C95B16B5ull, 0x28C054FB87ABF3E1ull,
     0xAA2724C0BA1A8096ull, 0xECA83EC980304F2Full},
    {0x6AA76EC294EB3303ull, 0x42D4CDB2A8032E3Bull, 0x7999EDF75DCD8735ull,
     0xB422BFFE696CCDCCull, 0x8F721461FD7CCDFEull, 0x148E1A5814FDE253ull,
     0x4DC941F4375EF8FFull, 0x27B2A9E0EB5B49CFull},
    {0xCEA592EF9343EBE1ull, 0xF7D38B5FA7698903ull, 0x6CCBF352203FEAB6ull,
     0x830F3095FCCDA9C5ull, 0xDBEEF4B81B81C8F4ull, 0x6D7EB9BCEECA5CF9ull,
     0xC58ABB0FBE436C69ull, 0xE4B97E6DB2041A4Bull},
    {0x7E40FC772978AF14ull, 0xCDDA4BBAE28354A1ull, 0xE4F993B832C32613ull,
     0xD3608093C68A4B35ull, 0x9A3B60E01BEE3699ull, 0x03BEF248F3288713ull,
     0x70B9294318F3E9B4ull, 0x8D2ABB913B8610DEull},
    {0x37F209128E7D8B2Cull, 0x81D2AB375BD874BCull, 0xA716A1B7373F7408ull,
     0x0CEE97BEC4706540ull, 0xA40C5FD9CDBC1512ull, 0x73CAF6C8918409E7ull,
     0x45E11BCEDF0BBAA1ull, 0x612C612BFF6E6605ull},
    {0xF8ECB14A12D0F649ull, 0xDA683CD7C01BA1ACull, 0xA2203F7510E124C1ull,
     0x7F83E52E162F3C78ull, 0x77D2BB73456ACADBull, 0x37FC34FC840BBA6Full,
     0x3076BC7D4C6EBC1Full, 0x4F514123632B5FA9ull},
    {0x44D789DED935E884ull, 0xF8291591E09FEC9Full, 0xD9CED2CF32A2E4B7ull,
     0x95F70E1EB604904Aull, 0xDE438FE43C14F6ABull, 0x4C8D23E4FAFCF8D8ull,
     0xC716910A3067EB86ull, 0x3D6B7915315095D3ull},
    {0x3170FDBADAB92095ull, 0x8F1963933FC5650Bull, 0x72F94F00ABECFEABull,
     0x6E3AE826C6AAB4CEull, 0xA677A2BF31068258ull, 0x9660CDC4F363AF10ull,
     0xD81A15A152379EF1ull, 0x5D7D285E1080A3F9ull},
    {0xDAD5DDFF9A2249B3ull, 0x6F9721D926103FAEull, 0x1418CBB83FFA349Aull,
     0xE71A30AD48C012B2ull, 0xBE76376C63751132ull, 0x3496467ACA713AE6ull,
     0x8D7EC01369F991A3ull, 0xD8C73A88B96B154Eull},
    {0x8B5D9C74AEB4833Aull, 0xF914FB3F867B912Full, 0xB894EA034936B1DCull,
     0x8A16D21BE51C4F5Bull, 0x31FF048ED582D98Eull, 0xB95AB2F4DC65B820ull,
     0x04082B9170561AF7ull, 0xA215610A5DC836FAull},
    {0xB2ADE592C092FAACull, 0x7A1E683BCBF13294ull, 0xC7A4DBF86858C096ull,
     0x3A49940F97BFF316ull, 0xCAE5C06B82C46703ull, 0xC7F413A0F951E2BDull,
     0x6665E7BB10EB5916ull, 0x86F84A5A94EDE319ull},
    {0x4EA199D8FAA79CA3ull, 0xDFA26E5BF1981704ull, 0x0F5E081D37FA4E01ull,
     0x9CB632F89CD675CDull, 0x4A09DB89D48C0304ull, 0x88142742EA3C7672ull,
     0xAC4F149E6D2E9BDBull, 0x6D9E1C23F8B1C6C6ull},
    {0xD58BE47B92DEC0E9ull, 0x8E57573645E34328ull, 0x4CC094CCB5FB5126ull,
     0x5F1D66AF6FB40E3Cull, 0x2BA15509132D3B00ull, 0x0D6545646120E567ull,
     0x3CF680C45C223666ull, 0x96B28E32930179DAull},
    {0x5900C45853AC7990ull, 0x61881E3E8B7FF169ull, 0x4DE5F835DF2230FFull,
     0x4427A9E7932F73FFull, 0x9B641BAD379A8C8Dull, 0xDF271E5BF98F4E5Cull,
     0xDFDA16DB830FF5EEull, 0x371C7E7CFB89C0E9ull},
    {0x4410A8576247A250ull, 0x6AD2DA12B45AC0D9ull, 0x18DFC72AAC85EECCull,
     0x06FC8BB2A0EF25C8ull, 0xEB287619C85E6118ull, 0x19553ECA67F25A2Cull,
     0x3B9557F1DCEC5BAAull, 0x7BAD9E8B710D1079ull},
    {0x34F365D66BD22B28ull, 0xE6E124B9F10F835Dull, 0x0573C38ABF2B24DCull,
     0xD32E6AF10A0125AEull, 0x383590ACEA979519ull, 0x8376ED7A39E28205ull,
     0xF0B7F184DCBDA435ull, 0x062A203390E31794ull},
    {0xA2AFFD7E41918760ull, 0x7F90FC1BD0819C86ull, 0x5033C08E5A969533ull,
     0x2707AF5C6D039590ull, 0x57BBD5980F17DF9Cull, 0xD3FE6E61D763268Aull,
     0x9E0A0AE40F335A3Bull, 0x43CF4EB0A99613C5ull},
    {0xD4D2A397CE1A7C2Eull, 0x3DF7CE7CC3212DADull, 0x0880F0D5D356C75Aull,
     0xA8AFC44DD03B1346ull, 0x79263B46C13A29E0ull, 0x11071B3C0ED58E7Aull,
     0xED46DC9F538406BFull, 0x2C94974F2B94843Dull},
    {0xE246E13C39AB5D5Eull, 0xAC1018489D955B20ull, 0x8601B558771852B8ull,
     0x110BD4C06DB40173ull, 0x738FC8A18CCA0EBBull, 0x6673E09BE0EA76E5ull,
     0x024BC7A0C7527877ull, 0x45E6B4652E2EC34Eull},
    {0xD1ED26A1A375CDC8ull, 0xAABC4E896A617CB8ull, 0x0A9C9E8E57D753C6ull,
     0xA3774A75FEB4C30Eull, 0x30B816C01C93E49Eull, 0xF405BABC06D2408Cull,
     0xCC0CE6B4CE788ABCull, 0x75E7922D0447956Cull},
    {0xD07C1676A698BC95ull, 0x5F9AEA4840E2D860ull, 0xD5FC10D58BDF6F02ull,
     0xF190A2AD4BC2EEA7ull, 0x0C24D11F51726931ull, 0xDB646899A16B6512ull,
     0x7BC10670047B1DD8ull, 0x2413A5ABCD45F092ull},
    {0x4E66892190CFD923ull, 0xF10162440365EC8Eull, 0x158ACA5A6A2280AEull,
     0x0D60ED11C0224166ull, 0x7CD2E9A71B9D7488ull, 0x450D7289706AB2A3ull,
     0x88FAE34EC9A0D7DCull, 0x96FF9103575A97DAull},
    {0x77990FAC6046C446ull, 0xB174B5FB30C76676ull, 0xE352CE3EB56CF82Aull,
     0xC6039B6873A9A082ull, 0xE3F80F3AE333148Aull, 0xB853BA24BA3539B9ull,
     0xE8863E52ECCB0C74ull, 0x309B4CC1092CC245ull},
    {0xBC2B70BEE8388D9Full, 0xE48D92AE22216DCEull, 0xF15F3BF3E2C15D8Full,
     0x1DD964D4812D8B24ull, 0xD56AF02FB4665E4Cull, 0x98002200595BD9A3ull,
     0x049246D50BB8FA12ull, 0x1B542DF485B579B9ull},
    {0x2347409ADFA8E497ull, 0x36015C2211D62498ull, 0xE9F141F32EB82690ull,
     0x1F839912D0449FB9ull, 0x4E4DCFFF2D02D97Cull, 0xF8A03AB4C0F625C9ull,
     0x0605F575795DAC5Cull, 0x4746C9BEA0DDA6B1ull},
    {0xCA5BB519ECE7481Bull, 0xFD496155E55CA945ull, 0xF753B9DBB1515F81ull,
     0x50549E8BAC0F70E7ull, 0x8614FB0271E21C60ull, 0x60C72947EB0F0070ull,
     0xA6511C10AEE742B6ull, 0x48FB48F2CACCB43Eull}};

#endif  // PRINT_RESULTS

// Ensures Xorshift128+ returns consistent and unchanging values.
void TestGolden() {
  HWY_ALIGN Xorshift128Plus rng(12345);
  for (uint64_t vector = 0; vector < kVectors; ++vector) {
    HWY_ALIGN uint64_t lanes[Xorshift128Plus::N];
    rng.Fill(lanes);
#if PRINT_RESULTS
    Print(lanes);
#else
    for (size_t i = 0; i < Xorshift128Plus::N; ++i) {
      ASSERT_EQ(kExpected[vector][i], lanes[i])
          << "Where vector=" << vector << " i=" << i;
    }
#endif
  }
}

// Output changes when given different seeds
void TestSeedChanges() {
  HWY_ALIGN uint64_t lanes[Xorshift128Plus::N];

  std::vector<uint64_t> first;
  constexpr size_t kNumSeeds = 16384;
  first.reserve(kNumSeeds);

  // All 14-bit seeds
  for (size_t seed = 0; seed < kNumSeeds; ++seed) {
    HWY_ALIGN Xorshift128Plus rng(seed);

    rng.Fill(lanes);
    first.push_back(lanes[0]);
  }

  // All outputs are unique
  ASSERT_EQ(kNumSeeds, first.size());
  std::sort(first.begin(), first.end());
  first.erase(std::unique(first.begin(), first.end()), first.end());
  EXPECT_EQ(kNumSeeds, first.size());
}

void TestFloat() {
  test::ThreadPoolForTests pool(8);

#ifdef JXL_DISABLE_SLOW_TESTS
  const uint32_t kMaxSeed = 256;
#else   // JXL_DISABLE_SLOW_TESTS
  const uint32_t kMaxSeed = 4096;
#endif  // JXL_DISABLE_SLOW_TESTS
  const auto test_seed = [](const uint32_t seed, size_t /*thread*/) -> Status {
    JxlMemoryManager* memory_manager = ::jxl::test::MemoryManager();
    HWY_ALIGN Xorshift128Plus rng(seed);

    const HWY_FULL(uint32_t) du;
    const HWY_FULL(float) df;
    size_t mem_size =
        Xorshift128Plus::N * sizeof(uint64_t) + Lanes(df) * sizeof(float);
    JXL_TEST_ASSIGN_OR_DIE(AlignedMemory mem,
                           AlignedMemory::Create(memory_manager, mem_size));
    uint64_t* batch = mem.address<uint64_t>();
    float* lanes = mem.address<float>() + Xorshift128Plus::N * 2;
    double sum = 0.0;
    size_t count = 0;
    const size_t kReps = 2000;
    for (size_t reps = 0; reps < kReps; ++reps) {
      rng.Fill(batch);
      for (size_t i = 0; i < Xorshift128Plus::N * 2; i += Lanes(df)) {
        const auto bits =
            Load(du, reinterpret_cast<const uint32_t*>(batch) + i);
        // 1.0 + 23 random mantissa bits = [1, 2)
        const auto rand12 =
            BitCast(df, Or(ShiftRight<9>(bits), Set(du, 0x3F800000)));
        const auto rand01 = Sub(rand12, Set(df, 1.0f));
        Store(rand01, df, lanes);
        for (size_t j = 0; j < Lanes(df); ++j) {
          float lane = lanes[j];
          sum += lane;
          count += 1;
          EXPECT_LE(lane, 1.0f);
          EXPECT_GE(lane, 0.0f);
        }
      }
    }

    // Verify average (uniform distribution)
    EXPECT_NEAR(0.5, sum / count, 0.00702);
    return true;
  };
  EXPECT_TRUE(RunOnPool(pool.get(), 0, kMaxSeed, ThreadPool::NoInit, test_seed,
                        "TestXorShift"));
}

// Not more than one 64-bit zero
void TestNotZero() {
  test::ThreadPoolForTests pool(8);

#ifdef JXL_DISABLE_SLOW_TESTS
  const uint32_t kMaxSeed = 500;
#else   // JXL_DISABLE_SLOW_TESTS
  const uint32_t kMaxSeed = 2000;
#endif  // JXL_DISABLE_SLOW_TESTS
  const auto test_seed = [](const uint32_t task, size_t /*thread*/) -> Status {
    HWY_ALIGN uint64_t lanes[Xorshift128Plus::N];

    HWY_ALIGN Xorshift128Plus rng(task);
    size_t num_zero = 0;
    for (size_t vectors = 0; vectors < 10000; ++vectors) {
      rng.Fill(lanes);
      for (uint64_t lane : lanes) {
        num_zero += static_cast<size_t>(lane == 0);
      }
    }
    EXPECT_LE(num_zero, 1u);
    return true;
  };
  EXPECT_TRUE(RunOnPool(pool.get(), 0, kMaxSeed, ThreadPool::NoInit, test_seed,
                        "TestNotZero"));
}

// NOLINTNEXTLINE(google-readability-namespace-comments)
}  // namespace HWY_NAMESPACE
}  // namespace jxl
HWY_AFTER_NAMESPACE();

#if HWY_ONCE
namespace jxl {

class Xorshift128Test : public hwy::TestWithParamTarget {};

HWY_TARGET_INSTANTIATE_TEST_SUITE_P(Xorshift128Test);

HWY_EXPORT_AND_TEST_P(Xorshift128Test, TestNotZero);
HWY_EXPORT_AND_TEST_P(Xorshift128Test, TestGolden);
HWY_EXPORT_AND_TEST_P(Xorshift128Test, TestSeedChanges);
HWY_EXPORT_AND_TEST_P(Xorshift128Test, TestFloat);

}  // namespace jxl
#endif
