<script>
    let { class: className = "", children } = $props();
</script>
<div class="{className} mb-3 flex w-full rounded-lg bg-gray-200 dark:bg-zinc-800 px-4 py-2">
    {@render children?.()}
</div>
