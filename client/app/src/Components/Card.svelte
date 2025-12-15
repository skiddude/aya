<script>
    let { title = null, icon = null, class: className = "", children, ...restProps } = $props()
    
    let classes = [className, "rounded-md border border-slate-200 bg-gray-50 dark:border-neutral-700 dark:bg-neutral-800/80"].filter(Boolean).join(" ")
</script>

<div class={classes} {...restProps}>
    {#if title === null && children?.title}
        {@render children.title()}
    {:else if title !== null}
        <div class="flex w-full select-none items-center rounded-t-md border-b border-slate-200 bg-gray-100 px-2 py-1 font-semibold dark:border-neutral-700 dark:bg-neutral-900 dark:text-neutral-50">
            <i class="fa-regular fa-light fa-fw {icon} me-1" />
            <span>{title}</span>
        </div>
    {/if}

    <div class="p-3">
        {@render children()}
    </div>
</div>