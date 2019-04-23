/*
 * (C) Copyright 2018
 * liye <ye.li@raycores.com>
 *
 * sdhlib.h - A description goes here.
 *
 */

#ifndef _HEAD_SDHLIB_0F701669_78B7086A_692E1DFD_H
#define _HEAD_SDHLIB_0F701669_78B7086A_692E1DFD_H

#include <stdint.h>
#include "common.h"

/* 1. 配置 local 规则端口组. 
* fpgaid : '1'-'4'
* portbitmap : bit0 表示该fpga的输出1端口 '1': 有效 '0': 无效 依次类推，最多配置4个输出端口 
* 返回值：'0' 成功 '!0' 失败
*/
int sdhlib_set_local_rule(struct proto_local_rule *rule);

/* 2. 配置 global 规则端口组. 
* 返回值：'0' 成功 '!0' 失败
*/
int sdhlib_set_global_rule(struct proto_global_rule *rule);

/* 3. 内部转发规则配置
* hp_port : 高阶输入端口号 '1'-'4' '13'-'16' 
* hp_chan : 高阶 输入端口中通道号 '1'-'64'
* sel : sel 值 '0'-'255'
* lp_chan : 低阶 通道号 '1'-'16'
* 返回值：'0' 成功 '!0' 失败
*/
int sdhlib_add_inner_sw_rule(uint8_t hp_port, uint8_t hp_chan, uint8_t sel, uint8_t lp_chan);
int sdhlib_del_inner_sw_rule(uint8_t hp_port, uint8_t hp_chan);

/* 4. 配置fpga的sel 
* 返回值：'0' 成功 '!0' 失败
*/
int sdhlib_set_sel(uint8_t fpgaid, uint8_t sel);

/* 5. 配置输出给客户的e1 
* 返回值：'0' 成功 '!0' 失败
*/
int sdhlib_set_e1user_map(struct proto_e1user_map *map);

/* 6. 配置linkmap的e1
* 返回值：'0' 成功 '!0' 失败
*/
int sdhlib_set_e1linkmap_map(struct proto_e1linkmap_map *map);

/* 7. 配置某个sel的权重（暂不实现）*/
int sdhlib_set_ratio(uint8_t sel, uint8_t ratio);

/* 8. 获取板卡的信息
* 返回值：'0' 成功 '!0' 失败
*/
int sdhlib_get_borad_info(struct board_info *bdinfo);

/* 9. 获取载荷信息
* 返回值：'0' 成功 '!0' 失败
*port : 高阶 指的是输入端口号‘1’-‘2’ 低阶指的是155M通道号 ‘1’-‘16’
*fpgaid : 高阶 ，低阶 ‘1’-‘4’
*/
int sdhlib_get_payload_info(uint8_t port, uint8_t fpgaid, struct payload_info *info);

/* 10. 获取高阶开销信息
* 返回值：'0' 成功 '!0' 失败
* port : 高阶 '1'-'4' '13'-'16' 低阶板卡该接口返回非0,失败。
*/
int sdhlib_get_soh_hpinfo(uint8_t port, struct soh_hpinfo *info);

/* 11. 获取低阶开销信息 
* 返回值：'0' 成功 '!0' 失败
* fpgaid : 低阶 ‘1’-‘4’
* port :  低阶 指的是155M通道号 ‘1’-‘16’
*/
int sdhlib_get_soh_lpinfo(uint8_t fpgaid, uint8_t port, struct soh_lpinfo *info);

/* 12. 获取每个载荷类型的解帧统计 
* 返回值：'0' 成功 '!0' 失败
*/
int sdhlib_get_board_stat(struct board_stat *stat);

/* 13. 获取gfp结果信息 
* 返回值：'0' 成功 '!0' 失败
* port : 高阶 指的是输入端口号‘1’-‘4'  低阶指的是155M通道号 ‘1’-‘16’
* fpgaid : 高阶  低阶 ‘1’-‘4’
* 输入文件（gfp result） /tmp/fpga{fpgaid}/sgfp{port_group 0-3}.cfg
*/
int sdhlib_get_gfp_info(uint8_t port,uint8_t fpgaid, struct gfp_info *info);

/*
* 14. sdhlib 初始化接口
*
*  返回值：'0' 成功 '!0' 失败
*
*/
int sdhlib_init(void);

/*
* 15. sdhlib 退出接口
*
*  返回值：'0' 成功 '!0' 失败
*
*/
int sdhlib_exit(void);

/* 16. 配置2m64kppp的接口
* 返回值：'0' 成功 '!0' 失败
*/
int sdhlib_set_2m64kppp_map(struct proto_2m64kppp_map *map);

/* 17. 配置板卡的信息
* 返回值：'0' 成功 '!0' 失败
*/
int sdhlib_set_borad_info(struct board_cfg_info *bdinfo);

/* 18. 获取端口状态
 * port : 1-24: front ports, switch ports(1-26) 25-40: fpga ports. 41-42: fab1/2 43-50: rear(1-8)
*  返回值：'0' 成功 '!0' 失败
* */
int sdhlib_get_port_status(uint8_t port, struct port_status *pstatus);

/* 19. 获取端口统计
 * port : 1-24: front ports, switch ports(1-26) 25-40: fpga ports. 41-42: fab1/2 43-50: rear(1-8)
*  返回值：'0' 成功 '!0' 失败
* */
int sdhlib_get_port_stat(uint8_t port, struct port_stat *pstat);

/* 20. 配置端口 单/双 纤
 * port : 1-24: front ports, switch ports(1-26) 25-40: fpga ports. 41-42: fab1/2 43-50: rear(1-8)
*  返回值：'0' 成功 '!0' 失败
* */
int sdhlib_set_port_fiber(uint8_t port, uint8_t fiber_type);

/* 21. 配置端口 mtu
 * port : 1-24: front ports, switch ports(1-26) 25-40: fpga ports. 41-42: fab1/2 43-50: rear(1-8)
* 返回值：'0' 成功 '!0' 失败
* */
int sdhlib_set_port_mtu(uint8_t port, uint32_t mtu);

/* 22. 配置端口 使能或是禁用
 * port : 1-24: front ports, switch ports(1-26) 25-40: fpga ports. 41-42: fab1/2 43-50: rear(1-8)
* 返回值：'0' 成功 '!0' 失败
* */
int sdhlib_set_port_enable(uint8_t port, uint8_t enable);

/* 23. 清除某个端口统计
 * port : 1-24: front ports, switch ports(1-26) 25-40: fpga ports. 41-42: fab1/2 43-50: rear(1-8)
* 返回值：'0' 成功 '!0' 失败
* */
int sdhlib_set_port_clear(uint8_t port);

/* 24. 配置 2mgfp 接口 
* 返回值：'0' 成功 '!0' 失败 */
int sdhlib_set_2mgfp_map(struct proto_2mgfp_map *map);

/* 25. 获取gfp的学习结果。 返回值：'0' 成功 '!0' 失败 */
int sdhlib_get_gfp_result(uint8_t fpgaid, struct gfp_result *result);

#if defined(__cplusplus)
extern "C" {
#endif

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef _HEAD_SDHLIB_0F701669_78B7086A_692E1DFD_H */
