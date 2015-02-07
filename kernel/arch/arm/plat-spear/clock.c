

#include <linux/bug.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <mach/misc_regs.h>
#include <plat/clock.h>

static DEFINE_SPINLOCK(clocks_lock);
static LIST_HEAD(root_clks);

static void propagate_rate(struct list_head *);

static int generic_clk_enable(struct clk *clk)
{
	unsigned int val;

	if (!clk->en_reg)
		return -EFAULT;

	val = readl(clk->en_reg);
	if (unlikely(clk->flags & RESET_TO_ENABLE))
		val &= ~(1 << clk->en_reg_bit);
	else
		val |= 1 << clk->en_reg_bit;

	writel(val, clk->en_reg);

	return 0;
}

static void generic_clk_disable(struct clk *clk)
{
	unsigned int val;

	if (!clk->en_reg)
		return;

	val = readl(clk->en_reg);
	if (unlikely(clk->flags & RESET_TO_ENABLE))
		val |= 1 << clk->en_reg_bit;
	else
		val &= ~(1 << clk->en_reg_bit);

	writel(val, clk->en_reg);
}

/* generic clk ops */
static struct clkops generic_clkops = {
	.enable = generic_clk_enable,
	.disable = generic_clk_disable,
};

int clk_enable(struct clk *clk)
{
	unsigned long flags;
	int ret = 0;

	if (!clk || IS_ERR(clk))
		return -EFAULT;

	spin_lock_irqsave(&clocks_lock, flags);
	if (clk->usage_count == 0) {
		if (clk->ops && clk->ops->enable)
			ret = clk->ops->enable(clk);
	}
	clk->usage_count++;
	spin_unlock_irqrestore(&clocks_lock, flags);

	return ret;
}
EXPORT_SYMBOL(clk_enable);

void clk_disable(struct clk *clk)
{
	unsigned long flags;

	if (!clk || IS_ERR(clk))
		return;

	WARN_ON(clk->usage_count == 0);

	spin_lock_irqsave(&clocks_lock, flags);
	clk->usage_count--;
	if (clk->usage_count == 0) {
		if (clk->ops && clk->ops->disable)
			clk->ops->disable(clk);
	}
	spin_unlock_irqrestore(&clocks_lock, flags);
}
EXPORT_SYMBOL(clk_disable);

unsigned long clk_get_rate(struct clk *clk)
{
	unsigned long flags, rate;

	spin_lock_irqsave(&clocks_lock, flags);
	rate = clk->rate;
	spin_unlock_irqrestore(&clocks_lock, flags);

	return rate;
}
EXPORT_SYMBOL(clk_get_rate);

int clk_set_parent(struct clk *clk, struct clk *parent)
{
	int i, found = 0, val = 0;
	unsigned long flags;

	if (!clk || IS_ERR(clk) || !parent || IS_ERR(parent))
		return -EFAULT;
	if (clk->usage_count)
		return -EBUSY;
	if (!clk->pclk_sel)
		return -EPERM;
	if (clk->pclk == parent)
		return 0;

	for (i = 0; i < clk->pclk_sel->pclk_count; i++) {
		if (clk->pclk_sel->pclk_info[i].pclk == parent) {
			found = 1;
			break;
		}
	}

	if (!found)
		return -EINVAL;

	spin_lock_irqsave(&clocks_lock, flags);
	/* reflect parent change in hardware */
	val = readl(clk->pclk_sel->pclk_sel_reg);
	val &= ~(clk->pclk_sel->pclk_sel_mask << clk->pclk_sel_shift);
	val |= clk->pclk_sel->pclk_info[i].pclk_mask << clk->pclk_sel_shift;
	writel(val, clk->pclk_sel->pclk_sel_reg);
	spin_unlock_irqrestore(&clocks_lock, flags);

	/* reflect parent change in software */
	clk->recalc(clk);
	propagate_rate(&clk->children);
	return 0;
}
EXPORT_SYMBOL(clk_set_parent);

/* registers clock in platform clock framework */
void clk_register(struct clk_lookup *cl)
{
	struct clk *clk = cl->clk;
	unsigned long flags;

	if (!clk || IS_ERR(clk))
		return;

	spin_lock_irqsave(&clocks_lock, flags);

	INIT_LIST_HEAD(&clk->children);
	if (clk->flags & ALWAYS_ENABLED)
		clk->ops = NULL;
	else if (!clk->ops)
		clk->ops = &generic_clkops;

	/* root clock don't have any parents */
	if (!clk->pclk && !clk->pclk_sel) {
		list_add(&clk->sibling, &root_clks);
		/* add clocks with only one parent to parent's children list */
	} else if (clk->pclk && !clk->pclk_sel) {
		list_add(&clk->sibling, &clk->pclk->children);
	} else {
		/* add clocks with > 1 parent to 1st parent's children list */
		list_add(&clk->sibling,
			 &clk->pclk_sel->pclk_info[0].pclk->children);
	}
	spin_unlock_irqrestore(&clocks_lock, flags);

	/* add clock to arm clockdev framework */
	clkdev_add(cl);
}

static void propagate_rate(struct list_head *lhead)
{
	struct clk *clkp, *_temp;

	list_for_each_entry_safe(clkp, _temp, lhead, sibling) {
		if (clkp->recalc)
			clkp->recalc(clkp);
		propagate_rate(&clkp->children);
	}
}

/* returns current programmed clocks clock info structure */
static struct pclk_info *pclk_info_get(struct clk *clk)
{
	unsigned int mask, i;
	unsigned long flags;
	struct pclk_info *info = NULL;

	spin_lock_irqsave(&clocks_lock, flags);
	mask = (readl(clk->pclk_sel->pclk_sel_reg) >> clk->pclk_sel_shift)
			& clk->pclk_sel->pclk_sel_mask;

	for (i = 0; i < clk->pclk_sel->pclk_count; i++) {
		if (clk->pclk_sel->pclk_info[i].pclk_mask == mask)
			info = &clk->pclk_sel->pclk_info[i];
	}
	spin_unlock_irqrestore(&clocks_lock, flags);

	return info;
}

static void change_parent(struct clk *cclk, struct clk *pclk)
{
	unsigned long flags;

	spin_lock_irqsave(&clocks_lock, flags);
	list_del(&cclk->sibling);
	list_add(&cclk->sibling, &pclk->children);

	cclk->pclk = pclk;
	spin_unlock_irqrestore(&clocks_lock, flags);
}

void pll1_clk_recalc(struct clk *clk)
{
	struct pll_clk_config *config = clk->private_data;
	unsigned int num = 2, den = 0, val, mode = 0;
	unsigned long flags;

	spin_lock_irqsave(&clocks_lock, flags);
	mode = (readl(config->mode_reg) >> PLL_MODE_SHIFT) &
		PLL_MODE_MASK;

	val = readl(config->cfg_reg);
	/* calculate denominator */
	den = (val >> PLL_DIV_P_SHIFT) & PLL_DIV_P_MASK;
	den = 1 << den;
	den *= (val >> PLL_DIV_N_SHIFT) & PLL_DIV_N_MASK;

	/* calculate numerator & denominator */
	if (!mode) {
		/* Normal mode */
		num *= (val >> PLL_NORM_FDBK_M_SHIFT) & PLL_NORM_FDBK_M_MASK;
	} else {
		/* Dithered mode */
		num *= (val >> PLL_DITH_FDBK_M_SHIFT) & PLL_DITH_FDBK_M_MASK;
		den *= 256;
	}

	clk->rate = (((clk->pclk->rate/10000) * num) / den) * 10000;
	spin_unlock_irqrestore(&clocks_lock, flags);
}

/* calculates current programmed rate of ahb or apb bus */
void bus_clk_recalc(struct clk *clk)
{
	struct bus_clk_config *config = clk->private_data;
	unsigned int div;
	unsigned long flags;

	spin_lock_irqsave(&clocks_lock, flags);
	div = ((readl(config->reg) >> config->shift) & config->mask) + 1;
	clk->rate = (unsigned long)clk->pclk->rate / div;
	spin_unlock_irqrestore(&clocks_lock, flags);
}

void aux_clk_recalc(struct clk *clk)
{
	struct aux_clk_config *config = clk->private_data;
	struct pclk_info *pclk_info = NULL;
	unsigned int num = 1, den = 1, val, eqn;
	unsigned long flags;

	/* get current programmed parent */
	pclk_info = pclk_info_get(clk);
	if (!pclk_info) {
		spin_lock_irqsave(&clocks_lock, flags);
		clk->pclk = NULL;
		clk->rate = 0;
		spin_unlock_irqrestore(&clocks_lock, flags);
		return;
	}

	change_parent(clk, pclk_info->pclk);

	spin_lock_irqsave(&clocks_lock, flags);
	if (pclk_info->scalable) {
		val = readl(config->synth_reg);

		eqn = (val >> AUX_EQ_SEL_SHIFT) & AUX_EQ_SEL_MASK;
		if (eqn == AUX_EQ1_SEL)
			den *= 2;

		/* calculate numerator */
		num = (val >> AUX_XSCALE_SHIFT) & AUX_XSCALE_MASK;

		/* calculate denominator */
		den *= (val >> AUX_YSCALE_SHIFT) & AUX_YSCALE_MASK;
		val = (((clk->pclk->rate/10000) * num) / den) * 10000;
	} else
		val = clk->pclk->rate;

	clk->rate = val;
	spin_unlock_irqrestore(&clocks_lock, flags);
}

void gpt_clk_recalc(struct clk *clk)
{
	struct aux_clk_config *config = clk->private_data;
	struct pclk_info *pclk_info = NULL;
	unsigned int div = 1, val;
	unsigned long flags;

	pclk_info = pclk_info_get(clk);
	if (!pclk_info) {
		spin_lock_irqsave(&clocks_lock, flags);
		clk->pclk = NULL;
		clk->rate = 0;
		spin_unlock_irqrestore(&clocks_lock, flags);
		return;
	}

	change_parent(clk, pclk_info->pclk);

	spin_lock_irqsave(&clocks_lock, flags);
	if (pclk_info->scalable) {
		val = readl(config->synth_reg);
		div += (val >> GPT_MSCALE_SHIFT) & GPT_MSCALE_MASK;
		div *= 1 << (((val >> GPT_NSCALE_SHIFT) & GPT_NSCALE_MASK) + 1);
	}

	clk->rate = (unsigned long)clk->pclk->rate / div;
	spin_unlock_irqrestore(&clocks_lock, flags);
}

void follow_parent(struct clk *clk)
{
	unsigned long flags;

	spin_lock_irqsave(&clocks_lock, flags);
	clk->rate = clk->pclk->rate;
	spin_unlock_irqrestore(&clocks_lock, flags);
}

void recalc_root_clocks(void)
{
	propagate_rate(&root_clks);
}
